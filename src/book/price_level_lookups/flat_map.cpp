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
    levels_.erase(price); // no-op if price is absent
}

PriceLevel* FlatMapPriceLevelLookup::min() const noexcept {
    if (levels_.empty()) return nullptr;
    return levels_.begin()->second;
}

PriceLevel* FlatMapPriceLevelLookup::max() const noexcept {
    if (levels_.empty()) return nullptr;
    return std::prev(levels_.end())->second;
}

std::size_t FlatMapPriceLevelLookup::size() const noexcept {
    return levels_.size();
}

bool FlatMapPriceLevelLookup::empty() const noexcept {
    return levels_.empty();
}

}