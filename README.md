# Order Book Engine

A C++20 market-data order book that reconstructs exchange state from
`ADD`, `CANCEL`, `REPLACE`, and `EXECUTE` events.

The project is intended as an exploration of data structures, memory layout,
and performance trade-offs in latency-sensitive trading systems.

---

## Features

The current implementation supports:

- Add, cancel, replace, and execute events
- Best bid and best ask queries
- Best bid / ask quantity
- Spread and mid-price calculation
- Quantity-at-price queries
- Bid and ask depth queries
- Individual order lookup by order ID
- FIFO ordering of orders within a price level
- Fixed-capacity order storage with recycled indices
- Pluggable price-level lookup implementations using C++20 concepts
- Unit tests with GoogleTest
- Performance benchmarks with Google Benchmark

---

## Architecture

At a high level, `OrderBook` coordinates four main pieces: the bid and ask
price-level lookups, `OrderLookup`, and `OrderPool`. Price-level lookups map
prices to `PriceLevel` objects, while `OrderLookup` maps exchange-facing order
IDs to the compact indices used by `OrderPool`. Each `PriceLevel` maintains
FIFO order using an intrusive doubly-linked list of those indices.

Together, these structures allow `OrderBook` to apply exchange events while
keeping the order store, order-ID index, and bid/ask price-level state
consistent.

---

## Core Types

Shared market types are defined in `types.hpp`:

```cpp
using OrderId    = std::uint64_t;
using Price      = std::int64_t;
using Quantity   = std::uint64_t;
using OrderIndex = std::uint32_t;
```

Prices are represented using an integer fixed-point/tick representation rather
than floating point.

Orders are addressed internally using 32-bit `OrderIndex` values rather than
raw pointers.

---

## Order

Each order stores its exchange-facing attributes together with links to the
previous and next orders at the same price level:

```cpp
struct Order {
    OrderId order_id;
    Price price;
    Quantity quantity;
    Side side;

    OrderIndex next;
    OrderIndex prev;
};
```

The `next` and `prev` fields form an intrusive doubly-linked list inside each
price level.

Using indices instead of pointers keeps order references compact and allows the
actual order objects to remain in contiguous storage owned by the `OrderPool`.

---

## OrderPool

`OrderPool` owns all order objects.

It is a fixed-capacity pool backed by contiguous `std::vector<Order>` storage.
The pool is initialized once and uses a freelist of `OrderIndex` values for
allocation and recycling. An `OrderIndex` directly identifies an entry in the
contiguous order storage. When an order is removed, its index is returned to
the freelist and can be reused by a later order.

### Why use a pool?

This avoids performing a separate heap allocation for every order after the
pool has been constructed.

It also keeps order storage contiguous and allows internal links to use compact
32-bit indices.

The current pool is intentionally simple:

- fixed capacity
- no allocation after construction
- constant-time allocation and release
- not thread-safe

---

## OrderLookup

Orders in an exchange feed are identified by `OrderId`, while the internal
storage is addressed using `OrderIndex`.

`OrderLookup` bridges the two by mapping each active `OrderId` to the
`OrderIndex` used to access that order in `OrderPool`.

The current implementation uses:

```cpp
std::unordered_map<OrderId, OrderIndex>
```

and reserves space up front based on the configured order capacity.

This abstraction is kept separate from `OrderPool` so that alternative order
lookup structures can be experimented with independently in the future.

---

## PriceLevel

A `PriceLevel` represents all resting orders at one price:

```cpp
struct PriceLevel {
    Price price;
    OrderIndex head;
    OrderIndex tail;
    std::uint32_t order_count;
};
```

Orders belonging to the same price are linked together as a FIFO
doubly-linked list. `head` and `tail` store the first and last `OrderIndex` in
the level, while each `Order` stores its neighboring indices.

Appending a new order and removing an existing order are therefore constant
time once the order and price level have been located.

The linked-list representation also avoids allocating a separate container for
the orders at every price level.

---

## PriceLevelLookup

The order book needs an ordered mapping from each price to its corresponding
`PriceLevel`. Rather than hard-coding one particular data structure into
`OrderBook`, the project defines a C++20 concept describing the operations a
price-level lookup must provide:

```cpp
template <typename T>
concept PriceLevelLookup = /* ... */;
```

A valid lookup implementation must support operations including:

- finding a price level
- inserting and removing a price level
- retrieving the minimum and maximum price
- finding the next price above or below a given price

The order book is then parameterized on the implementation:

```cpp
template <PriceLevelLookup Lookup>
class OrderBook;
```

This allows different lookup structures to be tested without changing the
core order-book logic.

---

## FlatMapPriceLevelLookup

The first price-level lookup implementation uses:

```cpp
boost::container::flat_map<Price, PriceLevel*>
```

Separate lookup instances are maintained for bids and asks.

Because the structure remains ordered by price:

- `max()` gives the best bid
- `min()` gives the best ask
- `next_below()` walks bid depth
- `next_above()` walks ask depth

`boost::container::flat_map` stores its mapping in contiguous storage, making
it an interesting initial implementation to compare against tree-based,
array-based, and custom structures later.

---

## OrderBook

`OrderBook` owns and coordinates:

```cpp
Lookup bids_;
Lookup asks_;

OrderLookup order_lookup_;
OrderPool order_pool_;
```

It provides two main interfaces:

1. applying exchange events
2. querying reconstructed market state

### Exchange Events

#### ADD

```cpp
apply_add(order_id, side, price, quantity);
```

An ADD event:

1. checks that the order ID is not already active
2. allocates an entry from `OrderPool`
3. initializes the order
4. inserts the `OrderId -> OrderIndex` mapping
5. finds or creates the appropriate price level
6. appends the order to the tail of the level's FIFO list

#### CANCEL

```cpp
apply_cancel(order_id);
```

A CANCEL event:

1. locates the order using `OrderLookup`
2. removes it from its price-level linked list
3. removes the order-ID mapping
4. releases the order back to the pool
5. removes the price level if it becomes empty

Cancelling an unknown order ID is currently treated as a no-op.

#### EXECUTE

```cpp
apply_execute(order_id, executed_quantity);
```

An execution reduces the remaining quantity of an existing order.

If the execution quantity consumes the entire remaining order, the order is
removed from the book.

#### REPLACE

```cpp
apply_replace(old_order_id, new_order_id, new_price, new_quantity);
```

A replace can modify:

- order ID
- price
- quantity

If the price changes, the order is removed from its previous price level and
appended to the new one.

If the order ID changes, the corresponding entry in `OrderLookup` is updated.

---

## Query API

The book exposes basic market-state primitives.

### Top of Book

```cpp
best_bid()
best_ask()

best_bid_qty()
best_ask_qty()

spread()
mid_price()
```

### Price Queries

```cpp
bid_qty(price)
ask_qty(price)

has_bid(price)
has_ask(price)
```

### Depth

```cpp
bids(depth)
asks(depth)
```

These return price/quantity views starting from the best price and walking
outward through the book.

### Order Query

```cpp
find_order(order_id)
```

This provides access to an individual resting order when order-level
information is required.

---

## Current Complexity Characteristics

For the current `FlatMapPriceLevelLookup` implementation:

- Order lookup is backed by `std::unordered_map`
- Orders at each price level are inserted/removed through an intrusive list
- Price levels are maintained in an ordered flat map
- Order storage allocation/release is handled by the fixed-capacity pool

One current design trade-off is that aggregate quantity is **not cached inside
`PriceLevel`**.

Queries such as:

```cpp
bid_qty(price)
ask_qty(price)
best_bid_qty()
best_ask_qty()
```

currently walk all orders at that level and sum their remaining quantities.

This keeps the initial representation simple, but maintaining aggregate
quantity directly inside each `PriceLevel` is one possible future
optimization.

---

## Testing

The project uses GoogleTest.

Tests currently cover:

- order allocation and recycling
- pool exhaustion
- order-ID lookup
- price-level lookup ordering
- minimum and maximum price retrieval
- ADD / CANCEL / EXECUTE / REPLACE behavior
- duplicate order IDs
- replacement with a new order ID
- partial and full executions
- best bid / best ask updates
- removal of empty price levels
- multiple orders at the same price
- bid and ask depth ordering
- linked-list removal from the head, middle, and tail

Tests can be run using:

```bash
ctest --test-dir build
```

or by executing the test binary directly.

---

## Benchmarking

The project includes an initial Google Benchmark harness.

The current benchmark generates a synthetic mixed stream containing:

- ADD
- CANCEL
- REPLACE
- EXECUTE

and interleaves book queries such as:

- best bid
- best ask
- spread
- limited bid depth

This benchmark is currently intended as a basic performance harness rather
than a rigorous latency study.

Future work will separate individual operations, measure latency
distributions, and replay more realistic market-data workloads.

---

## Building

### Requirements

- C++20-compatible compiler
- CMake >= 3.20
- Boost
- GoogleTest
- Google Benchmark

### Build

```bash
mkdir build
cd build
cmake ..
cmake --build .
```

### Run tests

```bash
ctest
```

### Run benchmark

```bash
./bench_book
```

---

## Project Structure

```text
orderbook-engine/
├── benchmarks/
│   └── bench_book.cpp
├── include/
│   └── book/
│       ├── order_book.hpp
│       ├── order.hpp
│       ├── order_lookup.hpp
│       ├── order_pool.hpp
│       ├── price_level.hpp
│       ├── price_level_lookup.hpp
│       ├── price_level_lookups/
│       │   └── flat_map.hpp
│       └── types.hpp
├── src/
│   └── book/
│       ├── order_book.cpp
│       ├── order_lookup.cpp
│       ├── order_pool.cpp
│       └── price_level_lookups/
│           └── flat_map.cpp
├── tests/
│   └── book/
│       ├── test_order_book.cpp
│       ├── test_order_lookup.cpp
│       ├── test_order_pool.cpp
│       └── price_level_lookups/
│           └── test_flat_map.cpp
└── CMakeLists.txt
```

---

## Motivation

The goal of this project is not to build a matching engine or simulate an
exchange.

The exchange is treated as the source of truth. The order book reconstructs
the state of the market from the event stream it receives.

The broader goal is to use the engine as a platform for studying the
interaction between:

- C++ data structures
- memory layout
- allocation
- cache locality
- algorithms
- concurrency
- networking
- operating-system behavior
- CPU architecture
- performance measurement

while working on a system representative of the kinds of components found in
electronic trading infrastructure.

---

## Roadmap

This project is a work in progress and is being extended incrementally. The
current implementation establishes the core single-threaded order book and its
main storage abstractions. Planned areas of exploration include:

### Alternative price-level lookup structures

The `PriceLevelLookup` abstraction was designed specifically to make this
possible.

Potential implementations include:

- tree-based structures
- B-trees
- indexed/direct-access arrays
- alternative flat containers
- custom data structures optimized for typical market-price distributions

The goal is to compare their behavior under realistic order-book workloads
rather than assume one structure is universally optimal.

### Alternative order-ID lookup structures

The current implementation uses `std::unordered_map`.

Future experiments may compare this against cache-friendly open-addressing or
dense hash-table implementations and potentially custom lookup structures.

### Price-level storage improvements

Possible extensions include:

- maintaining aggregate quantity directly inside each price level
- pooling price-level objects
- reducing dynamic allocation during level creation/deletion

### Realistic market-data replay

The current benchmark uses synthetically generated events.

A future replay pipeline will process real or realistic tick-by-tick market
data and reconstruct the book from a recorded event stream.

This will allow the engine to be benchmarked against more representative:

- price distributions
- order lifetimes
- add/cancel ratios
- execution patterns
- price-level churn

### More robust benchmarking

Planned benchmark work includes:

- per-operation benchmarks for ADD/CANCEL/REPLACE/EXECUTE
- throughput measurements
- latency distributions
- p50 / p99 / p99.9 latency
- warm-up and repeatability controls
- allocation measurements
- hardware performance counters
- cache-miss and branch-misprediction analysis

### Multi-threaded feed pipeline

A later version may separate market-data ingestion, parsing, order-book
updates, and read-side consumers across dedicated CPU cores, with an internal
queue connecting the producer and book-processing stages.

This would provide a practical environment for experimenting with:

- CPU affinity
- dedicated cores
- producer/consumer pipelines
- cache-line ownership
- false sharing
- thread synchronization
- latency between processing stages

### Lock-free communication

Communication between dedicated threads may use lock-free structures such as
an SPSC ring buffer.

This will allow experimentation with:

- atomics
- acquire/release ordering
- cache-line alignment
- false sharing
- bounded queues
- backpressure

### Networking

A future feed-handling layer may include a dedicated network thread responsible
for receiving and parsing market-data packets before publishing normalized
events to the order-book thread.

This will allow the project to explore the complete path from incoming market
data to reconstructed book state.
