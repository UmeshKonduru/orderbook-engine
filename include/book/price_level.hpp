#pragma once
#include <cstdint>
#include "book/types.hpp"

namespace book {

struct PriceLevel {
    Price price;
    OrderIndex head = kInvalidIndex;
    OrderIndex tail = kInvalidIndex;
    std::uint32_t order_count = 0;
};

}