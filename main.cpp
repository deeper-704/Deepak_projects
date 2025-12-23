#include "Orderbook.h"
#include "FixApp.h"

#include "quickfix/SessionSettings.h"
#include "quickfix/FileStore.h"
#include "quickfix/FileLog.h"
#include "quickfix/SocketAcceptor.h"

#include <iostream>
#include <csignal>
#include <atomic>
#include <thread>

static std::atomic<bool> running{true};

void signalHandler(int) {
    running = false;
}

int main() {
    try {
        // Handle Ctrl+C cleanly
        std::signal(SIGINT, signalHandler);
        std::signal(SIGTERM, signalHandler);

        // Load QuickFIX configuration
        FIX::SessionSettings settings("config/fix.cfg");

        // Core matching engine
        Orderbook orderbook;

        // FIX application
        FixApp application(orderbook);

        // Store & log factories
        FIX::FileStoreFactory storeFactory(settings);
        FIX::FileLogFactory logFactory(settings);

        // Acceptor (server-side FIX engine)
        FIX::SocketAcceptor acceptor(
            application,
            storeFactory,
            settings,
            logFactory
        );

        acceptor.start();
        std::cout << "FIX Order Matching Engine started." << std::endl;

        // Run until terminated
        while (running) {
            std::this_thread::sleep_for(std::chrono::seconds(1));
        }

        std::cout << "\nShutting down FIX engine..." << std::endl;
        acceptor.stop();

        return 0;
    }
    catch (const std::exception& e) {
        std::cerr << "Fatal error: " << e.what() << std::endl;
        return 1;
    }
}
