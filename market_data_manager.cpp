#include "market_data_manager.h"
#include <algorithm>
#include <chrono>
#include <thread>
#include <stdexcept>

MarketDataManager::MarketDataManager()
    : running_(false),
      config_manager_(ConfigManager::getInstance()) {
}

MarketDataManager::~MarketDataManager() {
    shutdown();
}

void MarketDataManager::initialize() {
    if (running_) return;
    
    running_ = true;
    processing_thread_ = std::thread(&MarketDataManager::processMarketData, this);
}

void MarketDataManager::shutdown() {
    if (!running_) return;
    
    running_ = false;
    if (processing_thread_.joinable()) {
        processing_thread_.join();
    }
}

void MarketDataManager::updateOrderBook(const OrderBook& orderbook) {
    std::lock_guard<std::mutex> lock(data_mutex_);
    
    auto& market_data = market_data_[orderbook.instrument];
    market_data.orderbook = orderbook;
    market_data.timestamp = std::chrono::system_clock::now();
    
    data_queue_.push(market_data);
}

void MarketDataManager::addTrade(const Trade& trade) {
    std::lock_guard<std::mutex> lock(data_mutex_);
    
    auto& market_data = market_data_[trade.instrument];
    market_data.trades.push_back(trade);
    
    // Keep only recent trades
    const size_t max_trades = 1000;
    if (market_data.trades.size() > max_trades) {
        market_data.trades.erase(market_data.trades.begin(),
                               market_data.trades.begin() + (market_data.trades.size() - max_trades));
    }
    
    market_data.timestamp = std::chrono::system_clock::now();
    data_queue_.push(market_data);
}

void MarketDataManager::updateMarketData(const MarketData& data) {
    std::lock_guard<std::mutex> lock(data_mutex_);
    market_data_[data.orderbook.instrument] = data;
    data_queue_.push(data);
}

const MarketDataManager::MarketData& MarketDataManager::getMarketData(const std::string& instrument) const {
    std::lock_guard<std::mutex> lock(data_mutex_);
    auto it = market_data_.find(instrument);
    if (it == market_data_.end()) {
        throw std::runtime_error("No market data available for instrument: " + instrument);
    }
    return it->second;
}

const MarketDataManager::OrderBook& MarketDataManager::getOrderBook(const std::string& instrument) const {
    return getMarketData(instrument).orderbook;
}

const std::vector<MarketDataManager::Trade>& MarketDataManager::getRecentTrades(const std::string& instrument, size_t count) const {
    std::lock_guard<std::mutex> lock(data_mutex_);
    auto it = market_data_.find(instrument);
    if (it == market_data_.end()) {
        throw std::runtime_error("No market data available for instrument: " + instrument);
    }
    
    const auto& trades = it->second.trades;
    if (trades.size() <= count) {
        return trades;
    }
    
    static std::vector<Trade> recent_trades;
    recent_trades.clear();
    recent_trades.insert(recent_trades.end(), trades.end() - count, trades.end());
    return recent_trades;
}

void MarketDataManager::subscribeToMarketData(const std::string& instrument,
                                            std::function<void(const MarketData&)> callback) {
    std::lock_guard<std::mutex> lock(data_mutex_);
    subscribers_[instrument].push_back(callback);
}

void MarketDataManager::unsubscribeFromMarketData(const std::string& instrument) {
    std::lock_guard<std::mutex> lock(data_mutex_);
    subscribers_.erase(instrument);
}

double MarketDataManager::getBestBid(const std::string& instrument) const {
    const auto& orderbook = getOrderBook(instrument);
    if (orderbook.bids.empty()) {
        throw std::runtime_error("No bids available for instrument: " + instrument);
    }
    return orderbook.bids.front().price;
}

double MarketDataManager::getBestAsk(const std::string& instrument) const {
    const auto& orderbook = getOrderBook(instrument);
    if (orderbook.asks.empty()) {
        throw std::runtime_error("No asks available for instrument: " + instrument);
    }
    return orderbook.asks.front().price;
}

double MarketDataManager::getMidPrice(const std::string& instrument) const {
    return (getBestBid(instrument) + getBestAsk(instrument)) / 2.0;
}

double MarketDataManager::getSpread(const std::string& instrument) const {
    return getBestAsk(instrument) - getBestBid(instrument);
}

void MarketDataManager::processMarketData() {
    while (running_) {
        MarketData data;
        {
            std::lock_guard<std::mutex> lock(data_mutex_);
            if (data_queue_.empty()) {
                continue;
            }
            data = data_queue_.front();
            data_queue_.pop();
        }
        
        notifySubscribers(data.orderbook.instrument, data);
        cleanupOldData();
        
        std::this_thread::sleep_for(std::chrono::milliseconds(1));
    }
}

void MarketDataManager::notifySubscribers(const std::string& instrument, const MarketData& data) {
    std::lock_guard<std::mutex> lock(data_mutex_);
    auto it = subscribers_.find(instrument);
    if (it != subscribers_.end()) {
        for (const auto& callback : it->second) {
            try {
                callback(data);
            } catch (...) {
                // Prevent subscriber exceptions from affecting other subscribers
            }
        }
    }
}

void MarketDataManager::cleanupOldData() {
    const auto now = std::chrono::system_clock::now();
    const auto max_age = std::chrono::minutes(5);
    
    std::lock_guard<std::mutex> lock(data_mutex_);
    for (auto it = market_data_.begin(); it != market_data_.end();) {
        if (now - it->second.timestamp > max_age) {
            it = market_data_.erase(it);
        } else {
            ++it;
        }
    }
} 