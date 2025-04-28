#ifndef TRADE_EXECUTION_H
#define TRADE_EXECUTION_H

#include "websocket_handler.h"
#include <nlohmann/json.hpp>
#include <string>
#include <functional>
#include <map>
#include <atomic>

// Forward declaration to avoid circular dependency
class WebSocketHandler;

using json = nlohmann::json;

class TradeExecution {
public:
   explicit TradeExecution(WebSocketHandler& websocket); // Constructor requiring WebSocketHandler reference
   ~TradeExecution(); // Destructor for cleanup

    // Order Management Functions
    json authenticate(const std::string& client_id, const std::string& client_secret);
    json getInstruments(const std::string& currency, const std::string& kind, bool expired);
    json placeBuyOrder(const std::string& instrument_name, double amount, double price);
    json cancelOrder(const std::string& order_id);
    json modifyOrder(const std::string& order_id, double new_price, double new_amount);
    json getOrderBook(const std::string& instrument_name);
    json getPositions();

    // Market Data Handling
    void handleMarketData(const json& data);
    void onMarketDataReceived(const json& market_data);

    // Subscriber Management
    void addMarketDataSubscriber(const std::string& symbol, std::function<void(const json&)> callback);

private:
   WebSocketHandler& websocket_;

    // Map to store subscribers and their callback functions
    std::map<std::string, std::function<void(const json&)>> market_data_subscribers_;

    // Atomic counter for generating unique JSON-RPC IDs
    static std::atomic<int> request_id;

    // Helper function to generate the next request ID
    int getNextRequestId();
};

#endif // TRADE_EXECUTION_H


