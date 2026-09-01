#include <benchmark/benchmark.h>
#include <vector>
#include <random>
#include "book/order_book.hpp"
#include "book/price_level_lookups/flat_map.hpp"

using namespace book;

struct Event {
    OrderId id;
    Price price;
    Quantity qty;
    Side side;
    uint8_t action; // 0: ADD, 1: CANCEL, 2: REPLACE, 3: EXECUTE
};

std::vector<Event> generate_mixed_flow(size_t count) {
    std::vector<Event> events;
    events.reserve(count);
    
    std::mt19937 gen(42);
    std::normal_distribution<> price_dist(10000.0, 50.0);
    std::uniform_int_distribution<> qty_dist(1, 100);
    std::uniform_int_distribution<> side_dist(0, 1);
    std::uniform_real_distribution<> action_dist(0.0, 1.0);

    for (size_t i = 1; i <= count; ++i) {
        Price p = static_cast<Price>(price_dist(gen));
        Quantity q = static_cast<Quantity>(qty_dist(gen));
        Side s = side_dist(gen) == 0 ? Side::Buy : Side::Sell;
        
        uint8_t action = 0; // Default ADD
        double r = action_dist(gen);
        if (i > 1000) { // Build initial book before mutating
            if (r < 0.4) action = 0;      // 40% ADD
            else if (r < 0.7) action = 1; // 30% CANCEL
            else if (r < 0.9) action = 2; // 20% REPLACE
            else action = 3;              // 10% EXECUTE
        }
        
        events.push_back({i, p, q, s, action});
    }
    return events;
}

static void BM_OrderBook_FullAPI(benchmark::State& state) {
    auto events = generate_mixed_flow(state.range(0));
    
    for (auto _ : state) {
        state.PauseTiming();
        OrderBook<FlatMapPriceLevelLookup> book(state.range(0) + 100);
        
        // Track active IDs to ensure valid replaces/cancels
        std::vector<OrderId> active_ids;
        active_ids.reserve(state.range(0));
        size_t active_idx = 0;
        state.ResumeTiming();

        for (const auto& ev : events) {
            if (ev.action == 0) {
                book.apply_add(ev.id, ev.side, ev.price, ev.qty);
                active_ids.push_back(ev.id);
            } 
            else if (active_idx < active_ids.size()) {
                OrderId target_id = active_ids[active_idx++];
                
                if (ev.action == 1) {
                    book.apply_cancel(target_id);
                } else if (ev.action == 2) {
                    book.apply_replace(target_id, target_id, ev.price, ev.qty);
                } else if (ev.action == 3) {
                    book.apply_execute(target_id, ev.qty / 2); // Partial fill
                }
            }

            // Force the compiler to evaluate the queries without optimizing them away
            benchmark::DoNotOptimize(book.best_bid());
            benchmark::DoNotOptimize(book.best_ask());
            benchmark::DoNotOptimize(book.spread());
            
            // Interleave a deeper query every 10th operation
            if (ev.id % 10 == 0) {
                benchmark::DoNotOptimize(book.bids(5));
            }
        }
    }
    state.SetItemsProcessed(state.iterations() * events.size());
}

BENCHMARK(BM_OrderBook_FullAPI)->Arg(10000)->Arg(100000)->Arg(500000);

BENCHMARK_MAIN();