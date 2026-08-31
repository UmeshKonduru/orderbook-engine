#pragma once
#include <cstddef>
#include <vector>
#include "book/order.hpp"
#include "book/types.hpp"

namespace book {

// Fixed-capacity pool. No heap allocation after construction.
// Not thread-safe. Double-release is undefined (no debug tracking, hot path).
class OrderPool {
public:
    explicit OrderPool(std::size_t capacity);

    OrderIndex allocate() noexcept;
    void release(OrderIndex index) noexcept;

    Order& get(OrderIndex index) noexcept;
    const Order& get(OrderIndex index) const noexcept;

    std::size_t capacity() const noexcept;
    std::size_t size() const noexcept;

private:
    std::vector<Order> orders_;
    std::vector<OrderIndex> free_list_;
};

}