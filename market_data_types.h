#ifndef MARKET_DATA_TYPES_H
#define MARKET_DATA_TYPES_H

#include <string>
#include <vector>
#include <chrono>

namespace market_data {

struct OrderBookLevel {
    double price;
    double size;
    std::chrono::system_clock::time_point timestamp;
};

struct OrderBook {
    std::vector<OrderBookLevel> bids;
    std::vector<OrderBookLevel> asks;
    std::chrono::system_clock::time_point timestamp;
    std::string instrument;
};

struct Trade {
    double price;
    double size;
    std::string side;
    std::string instrument;
    std::chrono::system_clock::time_point timestamp;
};

struct MarketData {
    OrderBook orderbook;
    std::vector<Trade> trades;
    double last_price;
    double volume_24h;
    double high_24h;
    double low_24h;
    std::chrono::system_clock::time_point timestamp;
};

} // namespace market_data

#endif // MARKET_DATA_TYPES_H 