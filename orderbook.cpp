#include <bits/stdc++.h>
using namespace std;

/* --------------------------
   Basic Types & Structures
   -------------------------- */
enum class ordertype { Limit, Market, FillAndKill };
enum class Side { Buy, Sell };

struct Levelinfo {
    int price;
    int quantity;
};
using Levelinfos = vector<Levelinfo>;

/* ----------------
   Order Class
   ---------------- */
class Order {
public:
    Order(ordertype type, int id, Side side, int price, int qty)
        : type_(type), id_(id), side_(side),
          price_(price), initial_qty_(qty), remaining_qty_(qty) {}

    int id() const { return id_; }
    Side side() const { return side_; }
    int price() const { return price_; }
    ordertype type() const { return type_; }
    int remaining() const { return remaining_qty_; }
    bool filled() const { return remaining_qty_ == 0; }

    void fill(int qty) {
        if (qty > remaining_qty_)
            throw runtime_error("Fill quantity exceeds remaining quantity");
        remaining_qty_ -= qty;
    }

private:
    ordertype type_;
    int id_;
    Side side_;
    int price_;
    int initial_qty_;
    int remaining_qty_;
};

using OrderPtr = shared_ptr<Order>;
using OrderList = list<OrderPtr>;

/* ----------------
   Trade Structures
   ---------------- */
struct TradeInfo {
    int order_id;
    int price;
    int quantity;
};

struct Trade {
    TradeInfo bid;
    TradeInfo ask;
};
using Trades = vector<Trade>;

/* ----------------
   OrderBook
   ---------------- */
class OrderBook {
private:
    map<int, OrderList, greater<int>> bids_;
    map<int, OrderList, less<int>> asks_;
    unordered_map<int, pair<OrderPtr, OrderList::iterator>> index_;

    bool canMatch(const OrderPtr& order) const {
        if (order->type() == ordertype::Market) return true;
        if (order->side() == Side::Buy && !asks_.empty())
            return order->price() >= asks_.begin()->first;
        if (order->side() == Side::Sell && !bids_.empty())
            return order->price() <= bids_.begin()->first;
        return false;
    }

    Trades match() {
        Trades trades;

        while (!bids_.empty() && !asks_.empty()) {
            auto& bidLevel = bids_.begin()->second;
            auto& askLevel = asks_.begin()->second;
            int bidPrice = bids_.begin()->first;
            int askPrice = asks_.begin()->first;

            if (bidPrice < askPrice) break;

            auto bid = bidLevel.front();
            auto ask = askLevel.front();

            int qty = min(bid->remaining(), ask->remaining());
            bid->fill(qty);
            ask->fill(qty);

            trades.push_back({
                {bid->id(), askPrice, qty},
                {ask->id(), askPrice, qty}
            });

            if (bid->filled()) {
                bidLevel.pop_front();
                index_.erase(bid->id());
            }
            if (ask->filled()) {
                askLevel.pop_front();
                index_.erase(ask->id());
            }

            if (bidLevel.empty()) bids_.erase(bidPrice);
            if (askLevel.empty()) asks_.erase(askPrice);
        }

        return trades;
    }

public:
    Trades addOrder(const OrderPtr& order) {
        if (index_.count(order->id())) return {};

        if (order->type() == ordertype::FillAndKill && !canMatch(order))
            return {};

        // Market orders never rest in book
        if (order->type() == ordertype::Market)
            return match();

        OrderList::iterator it;
        if (order->side() == Side::Buy) {
            auto& level = bids_[order->price()];
            level.push_back(order);
            it = prev(level.end());
        } else {
            auto& level = asks_[order->price()];
            level.push_back(order);
            it = prev(level.end());
        }

        index_[order->id()] = {order, it};

        auto trades = match();

        // FOK must be fully filled immediately
        if (order->type() == ordertype::FillAndKill && !order->filled())
            cancelOrder(order->id());

        return trades;
    }

    void cancelOrder(int id) {
        if (!index_.count(id)) return;
        auto [order, it] = index_[id];
        index_.erase(id);

        auto& book = (order->side() == Side::Buy) ? bids_ : asks_;
        auto& level = book[order->price()];
        level.erase(it);
        if (level.empty()) book.erase(order->price());
    }

    Levelinfos aggregate(const map<int, OrderList>& book) const {
        Levelinfos info;
        for (auto& [price, orders] : book) {
            int sum = 0;
            for (auto& o : orders) sum += o->remaining();
            info.push_back({price, sum});
        }
        return info;
    }

    void print() const {
        cout << "\nBIDS\n";
        for (auto& l : aggregate(bids_))
            cout << l.price << " " << l.quantity << "\n";
        cout << "\nASKS\n";
        for (auto& l : aggregate(asks_))
            cout << l.price << " " << l.quantity << "\n";
    }
};

/* ----------------
   Test Driver
   ---------------- */
int main() {
    OrderBook ob;

    ob.addOrder(make_shared<Order>(ordertype::Limit, 1, Side::Sell, 100, 5));
    ob.addOrder(make_shared<Order>(ordertype::Limit, 2, Side::Sell, 103, 5));
    ob.addOrder(make_shared<Order>(ordertype::Limit, 3, Side::Sell, 105, 5));

    ob.addOrder(make_shared<Order>(ordertype::Market, 4, Side::Buy, -1, 8));
    ob.addOrder(make_shared<Order>(ordertype::Limit, 5, Side::Buy, 102, 8));

    ob.print();
}
