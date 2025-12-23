#pragma once

#include "Orderbook.h"
#include "quickfix/Application.h"
#include "quickfix/MessageCracker.h"
#include "quickfix/fix44/NewOrderSingle.h"
#include "quickfix/fix44/ExecutionReport.h"

#include <iostream>

class FixApp : public FIX::Application, public FIX::MessageCracker {
public:
    explicit FixApp(Orderbook& ob) : orderbook_(ob) {}

    void onCreate(const FIX::SessionID& sessionID) override {
        std::cout << "Session created: " << sessionID << std::endl;
    }

    void onLogon(const FIX::SessionID& sessionID) override {
        std::cout << "Logon: " << sessionID << std::endl;
        sessionID_ = sessionID;
    }

    void onLogout(const FIX::SessionID& sessionID) override {
        std::cout << "Logout: " << sessionID << std::endl;
    }

    void fromAdmin(const FIX::Message&, const FIX::SessionID&) override {}
    void toAdmin(FIX::Message&, const FIX::SessionID&) override {}
    void toApp(FIX::Message&, const FIX::SessionID&) override {}

    void fromApp(const FIX::Message& message, const FIX::SessionID& sessionID) override {
        crack(message, sessionID);
    }

    /* -------------------------
       Handle NewOrderSingle
       ------------------------- */
    void onMessage(const FIX44::NewOrderSingle& msg,
                   const FIX::SessionID& sessionID) override {

        FIX::ClOrdID clOrdID;
        FIX::Side side;
        FIX::OrdType ordType;
        FIX::OrderQty qty;
        FIX::Symbol symbol;

        msg.get(clOrdID);
        msg.get(side);
        msg.get(ordType);
        msg.get(qty);
        msg.get(symbol);

        int id = std::stoi(clOrdID.getString());
        int quantity = qty.getValue();

        Side internalSide = (side == FIX::Side_BUY) ? Side::Buy : Side::Sell;

        ordertype internalType;
        int price = -1;

        if (ordType == FIX::OrdType_LIMIT) {
            FIX::Price px;
            msg.get(px);
            price = px.getValue();
            internalType = ordertype::Limit;
        } else if (ordType == FIX::OrdType_MARKET) {
            internalType = ordertype::Market;
        } else {
            sendReject(clOrdID, sessionID);
            return;
        }

        orderptr order = std::make_shared<Order>(
            internalType, id, internalSide, price, quantity
        );

        auto trades = orderbook_.addorder(order);

        sendNewAck(order, sessionID);

        for (const auto& t : trades) {
            sendFill(t, sessionID);
        }
    }

private:
    Orderbook& orderbook_;
    FIX::SessionID sessionID_;

    /* -------------------------
       Execution Reports
       ------------------------- */
    void sendNewAck(const orderptr& order,
                    const FIX::SessionID& sessionID) {

        FIX44::ExecutionReport report;
        report.set(FIX::OrderID("ORD" + std::to_string(order->getorderid())));
        report.set(FIX::ExecID("NEW"));
        report.set(FIX::ExecType(FIX::ExecType_NEW));
        report.set(FIX::OrdStatus(FIX::OrdStatus_NEW));
        report.set(FIX::LeavesQty(order->getrem()));
        report.set(FIX::CumQty(0));
        FIX::Session::sendToTarget(report, sessionID);
    }

    void sendFill(const trade& t,
                  const FIX::SessionID& sessionID) {

        FIX44::ExecutionReport report;
        report.set(FIX::ExecType(FIX::ExecType_TRADE));
        report.set(FIX::OrdStatus(FIX::OrdStatus_PARTIALLY_FILLED));
        report.set(FIX::LastQty(t.getbidtrade().quantity_));
        report.set(FIX::LastPx(t.getbidtrade().pprice_));
        FIX::Session::sendToTarget(report, sessionID);
    }

    void sendReject(const FIX::ClOrdID& clOrdID,
                    const FIX::SessionID& sessionID) {

        FIX44::ExecutionReport report;
        report.set(FIX::ClOrdID(clOrdID));
        report.set(FIX::ExecType(FIX::ExecType_REJECTED));
        report.set(FIX::OrdStatus(FIX::OrdStatus_REJECTED));
        FIX::Session::sendToTarget(report, sessionID);
    }
};
