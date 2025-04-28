#include "include/strategy_manager.h"
#include <algorithm>
#include <cmath>
#include <stdexcept>
#include <mutex>
#include <thread>
#include <vector>
#include <map>
#include <string>
#include <memory>

StrategyManager::StrategyManager()
    : config_manager_(ConfigManager::getInstance()),
      market_data_manager_(MarketDataManager::getInstance()),
      risk_manager_(RiskManager::getInstance()) {
}

StrategyManager::~StrategyManager() {
    shutdown();
}

void StrategyManager::initialize() {
    // Subscribe to market data updates
    market_data_manager_.subscribeToMarketData("BTC-PERPETUAL",
        [this](const MarketDataManager::MarketData& data) {
            processMarketData("BTC-PERPETUAL", data);
        });
}

void StrategyManager::shutdown() {
    // Unsubscribe from market data updates
    market_data_manager_.unsubscribeFromMarketData("BTC-PERPETUAL");
}

void StrategyManager::addStrategy(const StrategyConfig& config) {
    std::lock_guard<std::mutex> lock(strategy_mutex_);
    
    if (strategies_.find(config.name) != strategies_.end()) {
        throw std::runtime_error("Strategy already exists: " + config.name);
    }
    
    strategies_[config.name] = config;
    strategy_metrics_[config.name] = {
        0.0,    // total_pnl
        0.0,    // win_rate
        0.0,    // sharpe_ratio
        0.0,    // max_drawdown
        0,      // total_trades
        0,      // winning_trades
        std::chrono::system_clock::now()  // timestamp
    };
}

void StrategyManager::removeStrategy(const std::string& name) {
    std::lock_guard<std::mutex> lock(strategy_mutex_);
    strategies_.erase(name);
    strategy_metrics_.erase(name);
}

void StrategyManager::updateStrategy(const StrategyConfig& config) {
    std::lock_guard<std::mutex> lock(strategy_mutex_);
    
    if (strategies_.find(config.name) == strategies_.end()) {
        throw std::runtime_error("Strategy not found: " + config.name);
    }
    
    strategies_[config.name] = config;
}

void StrategyManager::enableStrategy(const std::string& name, bool enable) {
    std::lock_guard<std::mutex> lock(strategy_mutex_);
    
    auto it = strategies_.find(name);
    if (it == strategies_.end()) {
        throw std::runtime_error("Strategy not found: " + name);
    }
    
    it->second.enabled = enable;
}

const StrategyManager::StrategyConfig& StrategyManager::getStrategy(const std::string& name) const {
    std::lock_guard<std::mutex> lock(strategy_mutex_);
    
    auto it = strategies_.find(name);
    if (it == strategies_.end()) {
        throw std::runtime_error("Strategy not found: " + name);
    }
    
    return it->second;
}

const StrategyManager::StrategyMetrics& StrategyManager::getStrategyMetrics(const std::string& name) const {
    std::lock_guard<std::mutex> lock(strategy_mutex_);
    
    auto it = strategy_metrics_.find(name);
    if (it == strategy_metrics_.end()) {
        throw std::runtime_error("Strategy metrics not found: " + name);
    }
    
    return it->second;
}

std::vector<std::string> StrategyManager::getActiveStrategies() const {
    std::lock_guard<std::mutex> lock(strategy_mutex_);
    
    std::vector<std::string> active_strategies;
    for (const auto& [name, config] : strategies_) {
        if (config.enabled) {
            active_strategies.push_back(name);
        }
    }
    
    return active_strategies;
}

void StrategyManager::setStrategyCallback(std::function<void(const std::string&, const StrategyMetrics&)> callback) {
    std::lock_guard<std::mutex> lock(strategy_mutex_);
    strategy_callback_ = callback;
}

void StrategyManager::setTradeCallback(std::function<void(const std::string&, double, double, const std::string&)> callback) {
    std::lock_guard<std::mutex> lock(strategy_mutex_);
    trade_callback_ = callback;
}

void StrategyManager::processMarketData(const std::string& instrument, const MarketDataManager::MarketData& data) {
    std::lock_guard<std::mutex> lock(strategy_mutex_);
    
    for (const auto& [name, config] : strategies_) {
        if (config.enabled && config.instrument == instrument) {
            evaluateStrategy(name, data);
        }
    }
}

void StrategyManager::evaluateStrategy(const std::string& name, const MarketDataManager::MarketData& data) {
    const auto& config = strategies_[name];
    const auto& metrics = strategy_metrics_[name];
    
    // Check if we've reached the maximum number of trades for the day
    if (metrics.total_trades >= config.max_trades_per_day) {
        return;
    }
    
    double mid_price = market_data_manager_.getMidPrice(config.instrument);
    double spread = market_data_manager_.getSpread(config.instrument);
    
    // Example strategy: Mean reversion
    double price_deviation = (data.last_price - mid_price) / mid_price;
    
    if (std::abs(price_deviation) > config.entry_threshold) {
        double size = config.position_size;
        std::string side = price_deviation > 0 ? "sell" : "buy";
        
        if (risk_manager_.checkOrderRisk(config.instrument, size, data.last_price, side)) {
            executeTrade(name, size, data.last_price, side);
        }
    }
}

void StrategyManager::executeTrade(const std::string& strategy_name, double size, double price, const std::string& side) {
    // Here you would implement the actual trade execution logic
    // For now, we'll just update the metrics and notify callbacks
    
    double pnl = 0.0;  // This would be calculated based on the actual trade execution
    bool is_winning_trade = pnl > 0;
    
    updateStrategyMetrics(strategy_name, pnl, is_winning_trade);
    
    if (trade_callback_) {
        trade_callback_(strategy_name, size, price, side);
    }
}

void StrategyManager::updateStrategyMetrics(const std::string& name, double pnl, bool is_winning_trade) {
    auto& metrics = strategy_metrics_[name];
    
    metrics.total_pnl += pnl;
    metrics.total_trades++;
    if (is_winning_trade) {
        metrics.winning_trades++;
    }
    
    metrics.win_rate = static_cast<double>(metrics.winning_trades) / metrics.total_trades;
    metrics.timestamp = std::chrono::system_clock::now();
    
    // Update max drawdown
    if (pnl < 0) {
        metrics.max_drawdown = std::min(metrics.max_drawdown, pnl);
    }
    
    if (strategy_callback_) {
        strategy_callback_(name, metrics);
    }
} 