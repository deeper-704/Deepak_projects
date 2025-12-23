#pragma once

#include <map>
#include <list>
#include <vector>
#include <unordered_map>
#include <memory>
#include <numeric>
#include <mutex>

/* --------------------
   Core Types
   -------------------- */
enum class OrderType { Limit, Market, FillAndKill };
enum class Side { Buy, Sell };

/* --------------------
   Aggregated Levels
   -------------------- */
struct LevelInfo {
    int price;
    int quantity;
};

using LevelInfos = std::vector<LevelInfo>;

/* --------------------
   Order
   -------------------- */
class Order {
public:
    Order(OrderType type, int id, Side side, int price, int qty);

    int id() const;
    Side side() const;
    int price() const;
    OrderType type() const;
    int remaining() const;
    bool filled() const;

    void fill(int qty);

private:
    OrderType type_;
    int id_;
    Side side_;
    int price_;
    int remaining_;
};

using OrderPtr = std::shared_ptr<Order>;
using OrderList = std::list<OrderPtr>;

/* --------------------
   Trade Structures
   -------------------- */
struct TradeInfo {
    int order_id;
    int price;
    int quantity;
};

struct Trade {
    TradeInfo bid;
    TradeInfo ask;
};

using Trades = std::vector<Trade>;

/* --------------------
   Aggregated OrderBook
   -------------------- */
class AggregatedOrderBook {
public:
    AggregatedOrderBook(LevelInfos bids, LevelInfos asks);

    const LevelInfos& bids() const;
    const LevelInfos& asks() const;

private:
    LevelInfos bids_;
    LevelInfos asks_;
};

/* --------------------
   OrderBook
   -------------------- */
class OrderBook {
public:
    OrderBook();

    Trades addOrder(const OrderPtr& order);
    void cancelOrder(int orderId);

    AggregatedOrderBook snapshot() const;

private:
    Trades match();

    bool canMatch(const OrderPtr& order) const;

    std::map<int, OrderList, std::greater<int>> bids_;
    std::map<int, OrderList, std::less<int>> asks_;

    std::unordered_map<int, std::pair<OrderPtr, OrderList::iterator>> index_;

    mutable std::mutex mutex_;
};
