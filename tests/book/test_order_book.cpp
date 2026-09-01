#include <gtest/gtest.h>
#include "book/order_book.hpp"
#include "book/price_level_lookups/flat_map.hpp"

using book::DuplicateOrder;
using book::FlatMapPriceLevelLookup;
using book::OrderBook;
using book::OrderNotFound;
using book::Side;

using TestBook = OrderBook<FlatMapPriceLevelLookup>;

TEST(OrderBook, EmptyBookHasNoTopOfBook) {
    TestBook book(16);
    EXPECT_FALSE(book.best_bid().has_value());
    EXPECT_FALSE(book.best_ask().has_value());
    EXPECT_FALSE(book.spread().has_value());
    EXPECT_FALSE(book.mid_price().has_value());
}

TEST(OrderBook, AddThenCancelRemovesOrderAndLevel) {
    TestBook book(16);
    book.apply_add(1, Side::Buy, 100, 10);
    EXPECT_EQ(book.best_bid(), 100);
    EXPECT_EQ(book.best_bid_qty(), 10u);
    EXPECT_TRUE(book.has_bid(100));

    book.apply_cancel(1);
    EXPECT_FALSE(book.best_bid().has_value());
    EXPECT_FALSE(book.has_bid(100));
    EXPECT_EQ(book.find_order(1), nullptr);
}

TEST(OrderBook, PartialThenFullExecute) {
    TestBook book(16);
    book.apply_add(1, Side::Sell, 200, 10);
    book.apply_execute(1, 4);
    EXPECT_EQ(book.ask_qty(200), 6u);
    EXPECT_NE(book.find_order(1), nullptr);

    book.apply_execute(1, 6);
    EXPECT_EQ(book.ask_qty(200), 0u);
    EXPECT_FALSE(book.has_ask(200));
    EXPECT_EQ(book.find_order(1), nullptr);
}

TEST(OrderBook, ExecuteMoreThanRemainingFullyRemoves) {
    TestBook book(16);
    book.apply_add(1, Side::Buy, 100, 5);
    book.apply_execute(1, 100);
    EXPECT_FALSE(book.has_bid(100));
}

TEST(OrderBook, ReplaceChangesPriceAndMovesToNewLevel) {
    TestBook book(16);
    book.apply_add(1, Side::Buy, 100, 10);
    book.apply_add(2, Side::Buy, 100, 5);

    book.apply_replace(1, 1, 101, 7);

    EXPECT_EQ(book.bid_qty(100), 5u);
    EXPECT_EQ(book.bid_qty(101), 7u);
    EXPECT_EQ(book.best_bid(), 101);
}

TEST(OrderBook, ReplaceSamePriceKeepsLevelUpdatesQuantity) {
    TestBook book(16);
    book.apply_add(1, Side::Sell, 200, 10);
    book.apply_replace(1, 1, 200, 3);
    EXPECT_EQ(book.ask_qty(200), 3u);
    EXPECT_TRUE(book.has_ask(200));
}

TEST(OrderBook, ReplaceCanChangeOrderId) {
    TestBook book(16);
    book.apply_add(1, Side::Buy, 100, 10);
    book.apply_replace(1, 2, 100, 10);
    EXPECT_EQ(book.find_order(1), nullptr);
    EXPECT_NE(book.find_order(2), nullptr);
}

TEST(OrderBook, MultipleOrdersAtOneLevelAggregateQuantity) {
    TestBook book(16);
    book.apply_add(1, Side::Buy, 100, 5);
    book.apply_add(2, Side::Buy, 100, 7);
    book.apply_add(3, Side::Buy, 100, 3);
    EXPECT_EQ(book.bid_qty(100), 15u);
}

TEST(OrderBook, MultipleBidLevelsBestBidIsHighest) {
    TestBook book(16);
    book.apply_add(1, Side::Buy, 100, 5);
    book.apply_add(2, Side::Buy, 105, 5);
    book.apply_add(3, Side::Buy, 95, 5);
    EXPECT_EQ(book.best_bid(), 105);

    auto levels = book.bids(3);
    ASSERT_EQ(levels.size(), 3u);
    EXPECT_EQ(levels[0].price, 105);
    EXPECT_EQ(levels[1].price, 100);
    EXPECT_EQ(levels[2].price, 95);
}

TEST(OrderBook, MultipleAskLevelsBestAskIsLowest) {
    TestBook book(16);
    book.apply_add(1, Side::Sell, 200, 5);
    book.apply_add(2, Side::Sell, 195, 5);
    book.apply_add(3, Side::Sell, 210, 5);
    EXPECT_EQ(book.best_ask(), 195);

    auto levels = book.asks(3);
    ASSERT_EQ(levels.size(), 3u);
    EXPECT_EQ(levels[0].price, 195);
    EXPECT_EQ(levels[1].price, 200);
    EXPECT_EQ(levels[2].price, 210);
}

TEST(OrderBook, DepthShorterThanRequestedReturnsAvailableLevels) {
    TestBook book(16);
    book.apply_add(1, Side::Buy, 100, 5);
    auto levels = book.bids(5);
    ASSERT_EQ(levels.size(), 1u);
    EXPECT_EQ(levels[0].price, 100);
}

TEST(OrderBook, RemovalOfEmptyLevelClearsItFromLookup) {
    TestBook book(16);
    book.apply_add(1, Side::Buy, 100, 5);
    book.apply_add(2, Side::Buy, 105, 5);
    book.apply_cancel(2);
    EXPECT_FALSE(book.has_bid(105));
    EXPECT_EQ(book.best_bid(), 100);
}

TEST(OrderBook, BestBidChangesAsOrdersComeAndGo) {
    TestBook book(16);
    book.apply_add(1, Side::Buy, 100, 5);
    EXPECT_EQ(book.best_bid(), 100);
    book.apply_add(2, Side::Buy, 102, 5);
    EXPECT_EQ(book.best_bid(), 102);
    book.apply_cancel(2);
    EXPECT_EQ(book.best_bid(), 100);
}

TEST(OrderBook, BestAskChangesAsOrdersComeAndGo) {
    TestBook book(16);
    book.apply_add(1, Side::Sell, 200, 5);
    EXPECT_EQ(book.best_ask(), 200);
    book.apply_add(2, Side::Sell, 198, 5);
    EXPECT_EQ(book.best_ask(), 198);
    book.apply_cancel(2);
    EXPECT_EQ(book.best_ask(), 200);
}

TEST(OrderBook, HeadTailAndOnlyOrderRemoval) {
    TestBook book(16);
    book.apply_add(1, Side::Buy, 100, 1);
    book.apply_add(2, Side::Buy, 100, 2);
    book.apply_add(3, Side::Buy, 100, 3);
    // FIFO order: 1 (head), 2, 3 (tail)

    book.apply_cancel(1);  // remove head
    EXPECT_EQ(book.bid_qty(100), 5u);

    book.apply_cancel(3);  // remove tail
    EXPECT_EQ(book.bid_qty(100), 2u);

    book.apply_cancel(2);  // remove only remaining order
    EXPECT_FALSE(book.has_bid(100));
}

TEST(OrderBook, MiddleRemovalPreservesSurvivors) {
    TestBook book(16);
    book.apply_add(1, Side::Buy, 100, 1);
    book.apply_add(2, Side::Buy, 100, 2);
    book.apply_add(3, Side::Buy, 100, 3);

    book.apply_cancel(2);  // remove middle

    EXPECT_EQ(book.bid_qty(100), 4u);
    EXPECT_NE(book.find_order(1), nullptr);
    EXPECT_EQ(book.find_order(2), nullptr);
    EXPECT_NE(book.find_order(3), nullptr);
}

TEST(OrderBook, CancelOfMissingOrderIdIsNoop) {
    TestBook book(16);
    book.apply_add(1, Side::Buy, 100, 5);
    EXPECT_NO_THROW(book.apply_cancel(999));
    EXPECT_EQ(book.bid_qty(100), 5u);
}

TEST(OrderBook, ExecuteOfMissingOrderIdIsNoop) {
    TestBook book(16);
    EXPECT_NO_THROW(book.apply_execute(999, 5));
}

TEST(OrderBook, ReplaceOfMissingOrderIdThrows) {
    TestBook book(16);
    EXPECT_THROW(book.apply_replace(999, 1000, 100, 5), OrderNotFound);
}

TEST(OrderBook, AddDuplicateOrderIdThrows) {
    TestBook book(16);
    book.apply_add(1, Side::Buy, 100, 5);
    EXPECT_THROW(book.apply_add(1, Side::Buy, 101, 3), DuplicateOrder);
}

TEST(OrderBook, PoolExhaustionThrowsOnAdd) {
    TestBook book(2);
    book.apply_add(1, Side::Buy, 100, 1);
    book.apply_add(2, Side::Buy, 101, 1);
    EXPECT_THROW(book.apply_add(3, Side::Buy, 102, 1), book::PoolExhausted);
}