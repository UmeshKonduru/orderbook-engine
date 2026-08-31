#include <gtest/gtest.h>
#include "book/order_lookup.hpp"

using book::OrderLookup;
using book::kInvalidIndex;

TEST(OrderLookup, FindMissingReturnsInvalid) {
    OrderLookup lookup(1024);
    EXPECT_EQ(lookup.find(42), kInvalidIndex);
    EXPECT_FALSE(lookup.contains(42));
}

TEST(OrderLookup, InsertThenFind) {
    OrderLookup lookup(1024);
    lookup.insert(100, 5);
    EXPECT_EQ(lookup.find(100), 5u);
    EXPECT_TRUE(lookup.contains(100));
    EXPECT_EQ(lookup.size(), 1u);
}

TEST(OrderLookup, EraseRemovesEntry) {
    OrderLookup lookup(1024);
    lookup.insert(1, 0);
    lookup.erase(1);
    EXPECT_EQ(lookup.find(1), kInvalidIndex);
    EXPECT_EQ(lookup.size(), 0u);
}

TEST(OrderLookup, EraseMissingIsNoop) {
    OrderLookup lookup(1024);
    EXPECT_NO_THROW(lookup.erase(999));
    EXPECT_EQ(lookup.size(), 0u);
}

TEST(OrderLookup, MultipleEntriesIndependent) {
    OrderLookup lookup(1024);
    lookup.insert(1, 10);
    lookup.insert(2, 20);
    lookup.insert(3, 30);
    EXPECT_EQ(lookup.find(1), 10u);
    EXPECT_EQ(lookup.find(2), 20u);
    EXPECT_EQ(lookup.find(3), 30u);
    lookup.erase(2);
    EXPECT_EQ(lookup.find(2), kInvalidIndex);
    EXPECT_EQ(lookup.find(1), 10u);
    EXPECT_EQ(lookup.find(3), 30u);
    EXPECT_EQ(lookup.size(), 2u);
}

TEST(OrderLookup, ReinsertAfterEraseSucceeds) {
    OrderLookup lookup(1024);
    lookup.insert(7, 1);
    lookup.erase(7);
    lookup.insert(7, 2);
    EXPECT_EQ(lookup.find(7), 2u);
}