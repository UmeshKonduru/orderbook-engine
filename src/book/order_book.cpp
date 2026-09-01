#include "book/order_book.hpp"

#include <string>

#include "book/price_level_lookups/flat_map.hpp"

namespace book {

DuplicateOrder::DuplicateOrder(OrderId id)
    : std::runtime_error("duplicate live order id: " + std::to_string(id)) {}

OrderNotFound::OrderNotFound(OrderId id)
    : std::runtime_error("order id not found: " + std::to_string(id)) {}

PoolExhausted::PoolExhausted()
    : std::runtime_error("order pool exhausted") {}

template <PriceLevelLookup Lookup>
OrderBook<Lookup>::OrderBook(std::size_t order_capacity)
    : bids_(), asks_(), order_lookup_(order_capacity), order_pool_(order_capacity) {}

template <PriceLevelLookup Lookup>
OrderBook<Lookup>::~OrderBook() {
    auto destroy_side = [](Lookup& lookup) {
        PriceLevel* level = lookup.min();
        while (level != nullptr) {
            PriceLevel* next = lookup.next_above(level->price);
            delete level;
            level = next;
        }
    };
    destroy_side(bids_);
    destroy_side(asks_);
}

template <PriceLevelLookup Lookup>
Lookup& OrderBook<Lookup>::side_lookup(Side side) noexcept {
    return side == Side::Buy ? bids_ : asks_;
}

template <PriceLevelLookup Lookup>
const Lookup& OrderBook<Lookup>::side_lookup(Side side) const noexcept {
    return side == Side::Buy ? bids_ : asks_;
}

template <PriceLevelLookup Lookup>
Quantity OrderBook<Lookup>::level_qty(const PriceLevel* level, const OrderPool& pool) noexcept {
    if (level == nullptr) return 0;
    Quantity total = 0;
    OrderIndex idx = level->head;
    while (idx != kInvalidIndex) {
        const Order& order = pool.get(idx);
        total += order.quantity;
        idx = order.next;
    }
    return total;
}

template <PriceLevelLookup Lookup>
PriceLevel* OrderBook<Lookup>::get_or_create_level(Lookup& lookup, Price price) {
    PriceLevel* level = lookup.find(price);
    if (level != nullptr) return level;
    level = new PriceLevel{price};
    lookup.insert(price, level);
    return level;
}

template <PriceLevelLookup Lookup>
void OrderBook<Lookup>::append_order_to_level(PriceLevel* level, OrderIndex index) noexcept {
    Order& order = order_pool_.get(index);
    order.prev = level->tail;
    order.next = kInvalidIndex;
    if (level->tail != kInvalidIndex) {
        order_pool_.get(level->tail).next = index;
    } else {
        level->head = index;
    }
    level->tail = index;
    ++level->order_count;
}

template <PriceLevelLookup Lookup>
void OrderBook<Lookup>::remove_order_from_level(PriceLevel* level, OrderIndex index) noexcept {
    Order& order = order_pool_.get(index);
    OrderIndex prev = order.prev;
    OrderIndex next = order.next;

    if (prev != kInvalidIndex) {
        order_pool_.get(prev).next = next;
    } else {
        level->head = next;
    }

    if (next != kInvalidIndex) {
        order_pool_.get(next).prev = prev;
    } else {
        level->tail = prev;
    }

    order.prev = kInvalidIndex;
    order.next = kInvalidIndex;
    --level->order_count;
}

template <PriceLevelLookup Lookup>
void OrderBook<Lookup>::destroy_level_if_empty(Lookup& lookup, PriceLevel* level, Price price) noexcept {
    if (level->order_count == 0) {
        lookup.erase(price);
        delete level;
    }
}

template <PriceLevelLookup Lookup>
void OrderBook<Lookup>::erase_order(OrderId order_id, OrderIndex index, Side side, Price price) noexcept {
    Lookup& lookup = side_lookup(side);
    PriceLevel* level = lookup.find(price);
    remove_order_from_level(level, index);
    order_lookup_.erase(order_id);
    order_pool_.release(index);
    destroy_level_if_empty(lookup, level, price);
}

template <PriceLevelLookup Lookup>
void OrderBook<Lookup>::apply_add(OrderId order_id, Side side, Price price, Quantity quantity) {
    if (order_lookup_.contains(order_id)) {
        throw DuplicateOrder(order_id);
    }

    OrderIndex index = order_pool_.allocate();
    if (index == kInvalidIndex) {
        throw PoolExhausted();
    }

    Order& order = order_pool_.get(index);
    order.order_id = order_id;
    order.price = price;
    order.quantity = quantity;
    order.side = side;
    order.next = kInvalidIndex;
    order.prev = kInvalidIndex;

    order_lookup_.insert(order_id, index);

    Lookup& lookup = side_lookup(side);
    PriceLevel* level = get_or_create_level(lookup, price);
    append_order_to_level(level, index);
}

template <PriceLevelLookup Lookup>
void OrderBook<Lookup>::apply_cancel(OrderId order_id) noexcept {
    OrderIndex index = order_lookup_.find(order_id);
    if (index == kInvalidIndex) return;
    const Order& order = order_pool_.get(index);
    erase_order(order_id, index, order.side, order.price);
}

template <PriceLevelLookup Lookup>
void OrderBook<Lookup>::apply_execute(OrderId order_id, Quantity exec_quantity) noexcept {
    OrderIndex index = order_lookup_.find(order_id);
    if (index == kInvalidIndex) return;

    Order& order = order_pool_.get(index);
    if (exec_quantity >= order.quantity) {
        erase_order(order_id, index, order.side, order.price);
    } else {
        order.quantity -= exec_quantity;
    }
}

template <PriceLevelLookup Lookup>
void OrderBook<Lookup>::apply_replace(OrderId old_order_id, OrderId new_order_id,
                                       Price new_price, Quantity new_quantity) {
    OrderIndex index = order_lookup_.find(old_order_id);
    if (index == kInvalidIndex) {
        throw OrderNotFound(old_order_id);
    }
    if (new_order_id != old_order_id && order_lookup_.contains(new_order_id)) {
        throw DuplicateOrder(new_order_id);
    }

    Order& order = order_pool_.get(index);
    Side side = order.side;
    Price old_price = order.price;
    Lookup& lookup = side_lookup(side);

    if (new_price != old_price) {
        PriceLevel* old_level = lookup.find(old_price);
        remove_order_from_level(old_level, index);
        destroy_level_if_empty(lookup, old_level, old_price);

        order.price = new_price;
        PriceLevel* new_level = get_or_create_level(lookup, new_price);
        append_order_to_level(new_level, index);
    }

    order.quantity = new_quantity;

    if (new_order_id != old_order_id) {
        order_lookup_.erase(old_order_id);
        order.order_id = new_order_id;
        order_lookup_.insert(new_order_id, index);
    }
}

template <PriceLevelLookup Lookup>
std::optional<Price> OrderBook<Lookup>::best_bid() const noexcept {
    PriceLevel* level = bids_.max();
    if (level == nullptr) return std::nullopt;
    return level->price;
}

template <PriceLevelLookup Lookup>
std::optional<Price> OrderBook<Lookup>::best_ask() const noexcept {
    PriceLevel* level = asks_.min();
    if (level == nullptr) return std::nullopt;
    return level->price;
}

template <PriceLevelLookup Lookup>
std::optional<Quantity> OrderBook<Lookup>::best_bid_qty() const noexcept {
    PriceLevel* level = bids_.max();
    if (level == nullptr) return std::nullopt;
    return level_qty(level, order_pool_);
}

template <PriceLevelLookup Lookup>
std::optional<Quantity> OrderBook<Lookup>::best_ask_qty() const noexcept {
    PriceLevel* level = asks_.min();
    if (level == nullptr) return std::nullopt;
    return level_qty(level, order_pool_);
}

template <PriceLevelLookup Lookup>
std::optional<Price> OrderBook<Lookup>::spread() const noexcept {
    auto bid = best_bid();
    auto ask = best_ask();
    if (!bid.has_value() || !ask.has_value()) return std::nullopt;
    return *ask - *bid;
}

template <PriceLevelLookup Lookup>
std::optional<double> OrderBook<Lookup>::mid_price() const noexcept {
    auto bid = best_bid();
    auto ask = best_ask();
    if (!bid.has_value() || !ask.has_value()) return std::nullopt;
    return (static_cast<double>(*bid) + static_cast<double>(*ask)) / 2.0;
}

template <PriceLevelLookup Lookup>
Quantity OrderBook<Lookup>::bid_qty(Price price) const noexcept {
    return level_qty(bids_.find(price), order_pool_);
}

template <PriceLevelLookup Lookup>
Quantity OrderBook<Lookup>::ask_qty(Price price) const noexcept {
    return level_qty(asks_.find(price), order_pool_);
}

template <PriceLevelLookup Lookup>
bool OrderBook<Lookup>::has_bid(Price price) const noexcept {
    return bids_.contains(price);
}

template <PriceLevelLookup Lookup>
bool OrderBook<Lookup>::has_ask(Price price) const noexcept {
    return asks_.contains(price);
}

template <PriceLevelLookup Lookup>
std::vector<LevelView> OrderBook<Lookup>::collect_depth(const Lookup& lookup, bool descending,
                                                         std::size_t depth) const {
    std::vector<LevelView> result;
    result.reserve(depth);
    PriceLevel* level = descending ? lookup.max() : lookup.min();
    while (level != nullptr && result.size() < depth) {
        result.push_back(LevelView{level->price, level_qty(level, order_pool_)});
        level = descending ? lookup.next_below(level->price) : lookup.next_above(level->price);
    }
    return result;
}

template <PriceLevelLookup Lookup>
std::vector<LevelView> OrderBook<Lookup>::bids(std::size_t depth) const {
    return collect_depth(bids_, /*descending=*/true, depth);
}

template <PriceLevelLookup Lookup>
std::vector<LevelView> OrderBook<Lookup>::asks(std::size_t depth) const {
    return collect_depth(asks_, /*descending=*/false, depth);
}

template <PriceLevelLookup Lookup>
const Order* OrderBook<Lookup>::find_order(OrderId order_id) const noexcept {
    OrderIndex index = order_lookup_.find(order_id);
    if (index == kInvalidIndex) return nullptr;
    return &order_pool_.get(index);
}

// Explicit instantiation: the only PriceLevelLookup this project uses today.
// Add another `template class OrderBook<...>;` line if a second lookup
// implementation (btree, array, custom) needs to be instantiated.
template class OrderBook<FlatMapPriceLevelLookup>;

}