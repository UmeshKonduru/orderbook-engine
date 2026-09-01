#pragma once
#include <concepts>
#include <cstddef>
#include "book/price_level.hpp"
#include "book/types.hpp"

namespace book {

template <typename T>
concept PriceLevelLookup = requires(
    T lookup,
    const T clookup,
    Price price,
    PriceLevel* level
) {
    { clookup.find(price) }     -> std::same_as<PriceLevel*>;
    { clookup.contains(price) } -> std::convertible_to<bool>;

    { lookup.insert(price, level) } -> std::same_as<void>;
    { lookup.erase(price) }         -> std::same_as<void>;

    { clookup.min() } -> std::same_as<PriceLevel*>;
    { clookup.max() } -> std::same_as<PriceLevel*>;

    { clookup.next_below(price) } -> std::same_as<PriceLevel*>;
    { clookup.next_above(price) } -> std::same_as<PriceLevel*>;

    { clookup.size() }  -> std::same_as<std::size_t>;
    { clookup.empty() } -> std::same_as<bool>;
};

}