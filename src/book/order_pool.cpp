#include "book/order_pool.hpp"
#include <cassert>

namespace book {

OrderPool::OrderPool(std::size_t capacity)
    : orders_(capacity) {
    free_list_.reserve(capacity);
    for (std::size_t i = capacity; i-- > 0;) {
        free_list_.push_back(static_cast<OrderIndex>(i));
    }
}

OrderIndex OrderPool::allocate() noexcept {
    if (free_list_.empty()) [[unlikely]] {
        return kInvalidIndex;
    }
    
    OrderIndex idx = free_list_.back();
    free_list_.pop_back();
    return idx;
}

void OrderPool::release(OrderIndex index) noexcept {
    assert(index < orders_.size() && "OrderPool::release: Invalid index");
    free_list_.push_back(index);
}

Order& OrderPool::get(OrderIndex index) noexcept {
    assert(index < orders_.size() && "OrderPool::get: Index out of bounds");
    return orders_[index];
}

const Order& OrderPool::get(OrderIndex index) const noexcept {
    assert(index < orders_.size() && "OrderPool::get: Index out of bounds");
    return orders_[index];
}

std::size_t OrderPool::capacity() const noexcept {
    return orders_.size();
}

std::size_t OrderPool::size() const noexcept {
    return orders_.size() - free_list_.size();
}

}