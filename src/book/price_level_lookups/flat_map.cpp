#include "book/price_level_lookups/flat_map.hpp"

namespace book {

PriceLevel* FlatMapPriceLevelLookup::find(Price price) const noexcept {
    auto it = levels_.find(price);
    return it == levels_.end() ? nullptr : it->second;
}

bool FlatMapPriceLevelLookup::contains(Price price) const noexcept {
    return levels_.find(price) != levels_.end();
}

void FlatMapPriceLevelLookup::insert(Price price, PriceLevel* level) {
    levels_.insert_or_assign(price, level);
}

void FlatMapPriceLevelLookup::erase(Price price) noexcept {
    levels_.erase(price);
}

PriceLevel* FlatMapPriceLevelLookup::min() const noexcept {
    return levels_.empty() ? nullptr : levels_.begin()->second;
}

PriceLevel* FlatMapPriceLevelLookup::max() const noexcept {
    return levels_.empty() ? nullptr : levels_.rbegin()->second;
}

PriceLevel* FlatMapPriceLevelLookup::next_below(Price price) const noexcept {
    auto it = levels_.lower_bound(price);
    if (it == levels_.begin()) return nullptr;
    --it;
    return it->second;
}

PriceLevel* FlatMapPriceLevelLookup::next_above(Price price) const noexcept {
    auto it = levels_.upper_bound(price);
    return it == levels_.end() ? nullptr : it->second;
}

std::size_t FlatMapPriceLevelLookup::size() const noexcept {
    return levels_.size();
}

bool FlatMapPriceLevelLookup::empty() const noexcept {
    return levels_.empty();
}

}