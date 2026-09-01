#pragma once

#include <cstddef>
#include <optional>
#include <stdexcept>
#include <vector>

#include "book/order.hpp"
#include "book/order_lookup.hpp"
#include "book/order_pool.hpp"
#include "book/price_level.hpp"
#include "book/price_level_lookup.hpp"
#include "book/types.hpp"

namespace book {

class DuplicateOrder : public std::runtime_error {
public:
    explicit DuplicateOrder(OrderId id);
};

class OrderNotFound : public std::runtime_error {
public:
    explicit OrderNotFound(OrderId id);
};

class PoolExhausted : public std::runtime_error {
public:
    PoolExhausted();
};

struct LevelView {
    Price price;
    Quantity quantity;
};


template <PriceLevelLookup Lookup>
class OrderBook {
public:
    explicit OrderBook(std::size_t order_capacity);
    ~OrderBook();

    OrderBook(const OrderBook&) = delete;
    OrderBook& operator=(const OrderBook&) = delete;
    OrderBook(OrderBook&&) = delete;
    OrderBook& operator=(OrderBook&&) = delete;

    // --- Exchange event API ---------------------------------------------

    void apply_add(OrderId order_id, Side side, Price price, Quantity quantity);
    void apply_cancel(OrderId order_id) noexcept;
    void apply_execute(OrderId order_id, Quantity exec_quantity) noexcept;
    void apply_replace(OrderId old_order_id, OrderId new_order_id,
                        Price new_price, Quantity new_quantity);

    // --- Top of book -----------------------------------------------------

    std::optional<Price> best_bid() const noexcept;
    std::optional<Price> best_ask() const noexcept;

    std::optional<Quantity> best_bid_qty() const noexcept;
    std::optional<Quantity> best_ask_qty() const noexcept;

    std::optional<Price> spread() const noexcept;
    std::optional<double> mid_price() const noexcept;

    // --- Price queries -----------------------------------------------------

    Quantity bid_qty(Price price) const noexcept;
    Quantity ask_qty(Price price) const noexcept;

    bool has_bid(Price price) const noexcept;
    bool has_ask(Price price) const noexcept;

    // --- Depth -------------------------------------------------------------

    std::vector<LevelView> bids(std::size_t depth) const;
    std::vector<LevelView> asks(std::size_t depth) const;

    // --- Order query ---------------------------------------------------

    const Order* find_order(OrderId order_id) const noexcept;

private:
    Lookup bids_;
    Lookup asks_;

    OrderLookup order_lookup_;
    OrderPool order_pool_;

    Lookup& side_lookup(Side side) noexcept;
    const Lookup& side_lookup(Side side) const noexcept;

    static Quantity level_qty(const PriceLevel* level, const OrderPool& pool) noexcept;

    PriceLevel* get_or_create_level(Lookup& lookup, Price price);
    void append_order_to_level(PriceLevel* level, OrderIndex index) noexcept;
    void remove_order_from_level(PriceLevel* level, OrderIndex index) noexcept;
    void destroy_level_if_empty(Lookup& lookup, PriceLevel* level, Price price) noexcept;


    void erase_order(OrderId order_id, OrderIndex index, Side side, Price price) noexcept;

    std::vector<LevelView> collect_depth(const Lookup& lookup, bool descending, std::size_t depth) const;
};

}