#include <gtest/gtest.h>
#include "book/price_level_lookup.hpp"
#include "book/price_level_lookups/flat_map.hpp"

namespace book {
namespace {

PriceLevel make_level(Price p) {
    return PriceLevel{p, kInvalidIndex, kInvalidIndex, 0};
}

TEST(FlatMapPriceLevelLookup, EmptyState) {
    FlatMapPriceLevelLookup lookup;
    EXPECT_TRUE(lookup.empty());
    EXPECT_EQ(lookup.size(), 0u);
    EXPECT_EQ(lookup.find(100), nullptr);
    EXPECT_FALSE(lookup.contains(100));
    EXPECT_EQ(lookup.min(), nullptr);
    EXPECT_EQ(lookup.max(), nullptr);
}

TEST(FlatMapPriceLevelLookup, InsertAndFind) {
    FlatMapPriceLevelLookup lookup;
    PriceLevel level = make_level(100);

    lookup.insert(100, &level);

    EXPECT_FALSE(lookup.empty());
    EXPECT_EQ(lookup.size(), 1u);
    EXPECT_TRUE(lookup.contains(100));
    EXPECT_EQ(lookup.find(100), &level);
}

TEST(FlatMapPriceLevelLookup, FindMissingReturnsNullptr) {
    FlatMapPriceLevelLookup lookup;
    PriceLevel level = make_level(100);
    lookup.insert(100, &level);

    EXPECT_EQ(lookup.find(200), nullptr);
    EXPECT_FALSE(lookup.contains(200));
}

TEST(FlatMapPriceLevelLookup, DuplicatePriceReplacesMapping) {
    FlatMapPriceLevelLookup lookup;
    PriceLevel level_a = make_level(100);
    PriceLevel level_b = make_level(100);

    lookup.insert(100, &level_a);
    lookup.insert(100, &level_b);

    EXPECT_EQ(lookup.size(), 1u);
    EXPECT_EQ(lookup.find(100), &level_b);
}

TEST(FlatMapPriceLevelLookup, EraseExisting) {
    FlatMapPriceLevelLookup lookup;
    PriceLevel level = make_level(100);
    lookup.insert(100, &level);

    lookup.erase(100);

    EXPECT_TRUE(lookup.empty());
    EXPECT_EQ(lookup.find(100), nullptr);
    EXPECT_FALSE(lookup.contains(100));
}

TEST(FlatMapPriceLevelLookup, EraseMissingIsNoOp) {
    FlatMapPriceLevelLookup lookup;
    PriceLevel level = make_level(100);
    lookup.insert(100, &level);

    lookup.erase(999); // not present

    EXPECT_EQ(lookup.size(), 1u);
    EXPECT_TRUE(lookup.contains(100));
}

TEST(FlatMapPriceLevelLookup, EraseOnEmptyIsNoOp) {
    FlatMapPriceLevelLookup lookup;
    lookup.erase(100); // should not throw or crash
    EXPECT_TRUE(lookup.empty());
}

TEST(FlatMapPriceLevelLookup, MinMaxSingleLevel) {
    FlatMapPriceLevelLookup lookup;
    PriceLevel level = make_level(150);
    lookup.insert(150, &level);

    EXPECT_EQ(lookup.min(), &level);
    EXPECT_EQ(lookup.max(), &level);
}

TEST(FlatMapPriceLevelLookup, MinMaxMultipleLevels) {
    FlatMapPriceLevelLookup lookup;
    PriceLevel low  = make_level(100);
    PriceLevel mid  = make_level(150);
    PriceLevel high = make_level(200);

    // insert out of order to check flat_map ordering, not insertion order
    lookup.insert(150, &mid);
    lookup.insert(200, &high);
    lookup.insert(100, &low);

    EXPECT_EQ(lookup.size(), 3u);
    EXPECT_EQ(lookup.min(), &low);
    EXPECT_EQ(lookup.max(), &high);
}

TEST(FlatMapPriceLevelLookup, MinMaxAfterErasingBoundary) {
    FlatMapPriceLevelLookup lookup;
    PriceLevel low  = make_level(100);
    PriceLevel mid  = make_level(150);
    PriceLevel high = make_level(200);
    lookup.insert(100, &low);
    lookup.insert(150, &mid);
    lookup.insert(200, &high);

    lookup.erase(200);

    EXPECT_EQ(lookup.max(), &mid);
    EXPECT_EQ(lookup.min(), &low);
}

TEST(FlatMapPriceLevelLookup, BecomesEmptyAfterErasingAll) {
    FlatMapPriceLevelLookup lookup;
    PriceLevel a = make_level(100);
    PriceLevel b = make_level(200);
    lookup.insert(100, &a);
    lookup.insert(200, &b);

    lookup.erase(100);
    lookup.erase(200);

    EXPECT_TRUE(lookup.empty());
    EXPECT_EQ(lookup.min(), nullptr);
    EXPECT_EQ(lookup.max(), nullptr);
}

TEST(FlatMapPriceLevelLookup, NegativePricesOrderCorrectly) {
    // Price is a signed fixed-point tick type; verify ordering isn't
    // accidentally treated as unsigned.
    FlatMapPriceLevelLookup lookup;
    PriceLevel neg = make_level(-50);
    PriceLevel pos = make_level(50);
    lookup.insert(50, &pos);
    lookup.insert(-50, &neg);

    EXPECT_EQ(lookup.min(), &neg);
    EXPECT_EQ(lookup.max(), &pos);
}

TEST(FlatMapPriceLevelLookup, SatisfiesConceptStatically) {
    static_assert(PriceLevelLookup<FlatMapPriceLevelLookup>);
}

}
}