#ifndef MARKET_DATA_MANAGER_H
#define MARKET_DATA_MANAGER_H

#include <string>
#include <vector>
#include <map>
#include <mutex>
#include <queue>
#include <memory>
#include <functional>
#include <chrono>
#include <atomic>
#include "config_manager.h"

class MarketDataManager {
public:
    struct OrderBook {
        struct Level {
            double price;
            double size;
            std::chrono::system_clock::time_point timestamp;
        };

        std::vector<Level> bids;
        std::vector<Level> asks;
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

    static MarketDataManager& getInstance() {
        static MarketDataManager instance;
        return instance;
    }

    void initialize();
    void shutdown();

    void updateOrderBook(const OrderBook& orderbook);
    void addTrade(const Trade& trade);
    void updateMarketData(const MarketData& data);

    const MarketData& getMarketData(const std::string& instrument) const;
    const OrderBook& getOrderBook(const std::string& instrument) const;
    const std::vector<Trade>& getRecentTrades(const std::string& instrument, size_t count = 10) const;

    void subscribeToMarketData(const std::string& instrument,
                             std::function<void(const MarketData&)> callback);
    void unsubscribeFromMarketData(const std::string& instrument);

    double getBestBid(const std::string& instrument) const;
    double getBestAsk(const std::string& instrument) const;
    double getMidPrice(const std::string& instrument) const;
    double getSpread(const std::string& instrument) const;

private:
    MarketDataManager();
    ~MarketDataManager();
    MarketDataManager(const MarketDataManager&) = delete;
    MarketDataManager& operator=(const MarketDataManager&) = delete;

    void processMarketData();
    void notifySubscribers(const std::string& instrument, const MarketData& data);
    void cleanupOldData();

    mutable std::mutex data_mutex_;
    std::map<std::string, MarketData> market_data_;
    std::map<std::string, std::vector<std::function<void(const MarketData&)>>> subscribers_;
    std::queue<MarketData> data_queue_;
    std::atomic<bool> running_;
    std::thread processing_thread_;
    const ConfigManager& config_manager_;
};

#endif // MARKET_DATA_MANAGER_H 