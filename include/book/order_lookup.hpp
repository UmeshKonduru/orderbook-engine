#pragma once
#include <cstddef>
#include <unordered_map>
#include "book/types.hpp"

namespace book {

class OrderLookup {
public:
    explicit OrderLookup(std::size_t capacity);

    OrderIndex find(OrderId id) const noexcept;
    bool contains(OrderId id) const noexcept;

    void insert(OrderId id, OrderIndex index);
    void erase(OrderId id) noexcept;

    std::size_t size() const noexcept;

private:
    std::unordered_map<OrderId, OrderIndex> map_;
};

}