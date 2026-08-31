#pragma once
#include <cstdint>

namespace book {

using OrderId    = std::uint64_t;
using Price      = std::int64_t;   // fixed-point ticks, not float
using Quantity   = std::uint64_t;
using OrderIndex = std::uint32_t;

inline constexpr OrderIndex kInvalidIndex = static_cast<OrderIndex>(-1);

enum class Side : std::uint8_t {
    Buy,
    Sell
};

}