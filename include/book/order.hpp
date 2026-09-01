#pragma once
#include "book/types.hpp"

namespace book {

struct Order {
    OrderId  order_id;
    Price    price;
    Quantity quantity;
    Side     side;

    OrderIndex next = kInvalidIndex;
    OrderIndex prev = kInvalidIndex;
};

}