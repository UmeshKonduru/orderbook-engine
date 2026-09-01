#pragma once
#include <cstddef>
#include <boost/container/flat_map.hpp>
#include "book/price_level.hpp"
#include "book/types.hpp"

namespace book {

class FlatMapPriceLevelLookup {
public:
    FlatMapPriceLevelLookup() = default;

    PriceLevel* find(Price price) const noexcept;
    bool contains(Price price) const noexcept;

    void insert(Price price, PriceLevel* level);
    void erase(Price price) noexcept;

    PriceLevel* min() const noexcept;
    PriceLevel* max() const noexcept;

    std::size_t size() const noexcept;
    bool empty() const noexcept;

private:
    boost::container::flat_map<Price, PriceLevel*> levels_;
};

}