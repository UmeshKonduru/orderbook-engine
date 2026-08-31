#include "book/order_lookup.hpp"
#include <cassert>

namespace book {

OrderLookup::OrderLookup(std::size_t capacity) {
    map_.reserve(capacity);
}

OrderIndex OrderLookup::find(OrderId id) const noexcept {
    auto it = map_.find(id);
    return it == map_.end() ? kInvalidIndex : it->second;
}

bool OrderLookup::contains(OrderId id) const noexcept {
    return map_.find(id) != map_.end();
}

void OrderLookup::insert(OrderId id, OrderIndex index) {
    assert(map_.find(id) == map_.end() && "OrderLookup::insert: duplicate OrderId insert");
    map_.emplace(id, index);
}

void OrderLookup::erase(OrderId id) noexcept {
    map_.erase(id);
}

std::size_t OrderLookup::size() const noexcept {
    return map_.size();
}

}