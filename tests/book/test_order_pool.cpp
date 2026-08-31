#include <gtest/gtest.h>
#include "book/order_pool.hpp"
#include "book/types.hpp"

using book::Order;
using book::OrderIndex;
using book::OrderPool;
using book::kInvalidIndex;

TEST(OrderPoolTest, AllocateReturnsDistinctIndices) {
    OrderPool pool(4);
    OrderIndex a = pool.allocate();
    OrderIndex b = pool.allocate();
    EXPECT_NE(a, b);
}

TEST(OrderPoolTest, SizeTracksAllocations) {
    OrderPool pool(4);
    EXPECT_EQ(pool.size(), 0u);
    OrderIndex a = pool.allocate();
    EXPECT_EQ(pool.size(), 1u);
    pool.release(a);
    EXPECT_EQ(pool.size(), 0u);
}

TEST(OrderPoolTest, ReleasedIndexIsRecycled) {
    OrderPool pool(2);
    OrderIndex a = pool.allocate();
    pool.release(a);
    OrderIndex b = pool.allocate();
    EXPECT_EQ(a, b);
}

TEST(OrderPoolTest, GetReturnsWritableOrder) {
    OrderPool pool(1);
    OrderIndex idx = pool.allocate();
    pool.get(idx).order_id = 42;
    EXPECT_EQ(pool.get(idx).order_id, 42ull); 
}

TEST(OrderPoolTest, AllocateBeyondCapacityReturnsInvalid) {
    OrderPool pool(1);
    pool.allocate();
    EXPECT_EQ(pool.allocate(), kInvalidIndex);
}

TEST(OrderPoolTest, ReleaseInvalidIndexAsserts) {
    OrderPool pool(1);
    EXPECT_DEBUG_DEATH(pool.release(999), "Invalid index");
}

TEST(OrderPoolTest, GetInvalidIndexAsserts) {
    OrderPool pool(1);
    EXPECT_DEBUG_DEATH(pool.get(999), "Index out of bounds");
}