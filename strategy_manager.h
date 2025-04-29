#ifndef STRATEGY_MANAGER_H
#define STRATEGY_MANAGER_H

// C++ Standard Library includes
#include <string>
#include <vector>
#include <map>
#include <memory>
#include <functional>
#include <chrono>
#include <thread>
#include <algorithm>

// Boost includes
#include <boost/thread/mutex.hpp>

// Project includes
#include "market_data_types.h"

// Forward declarations
class ConfigManager;
class MarketDataManager;
class RiskManager;

class StrategyManager {
public:
    struct StrategyConfig {
        std::string name;
        std::string instrument;
        double position_size;
        double entry_threshold;
        double exit_threshold;
        double stop_loss;
        double take_profit;
        int max_trades_per_day;
        bool enabled;
    };

    struct StrategyMetrics {
        double total_pnl;
        double win_rate;
        double sharpe_ratio;
        double max_drawdown;
        int total_trades;
        int winning_trades;
        std::chrono::system_clock::time_point timestamp;
    };

    static StrategyManager& getInstance() {
        static StrategyManager instance;
        return instance;
    }

    void initialize();
    void shutdown();

    void addStrategy(const StrategyConfig& config);
    void removeStrategy(const std::string& name);
    void updateStrategy(const StrategyConfig& config);
    void enableStrategy(const std::string& name, bool enable);

    const StrategyConfig& getStrategy(const std::string& name) const;
    const StrategyMetrics& getStrategyMetrics(const std::string& name) const;
    std::vector<std::string> getActiveStrategies() const;

    void setStrategyCallback(std::function<void(const std::string&, const StrategyMetrics&)> callback);
    void setTradeCallback(std::function<void(const std::string&, double, double, const std::string&)> callback);

private:
    StrategyManager();
    ~StrategyManager();
    StrategyManager(const StrategyManager&) = delete;
    StrategyManager& operator=(const StrategyManager&) = delete;

    void processMarketData(const std::string& instrument, const market_data::MarketData& data);
    void evaluateStrategy(const std::string& name, const market_data::MarketData& data);
    void executeTrade(const std::string& strategy_name, double size, double price, const std::string& side);
    void updateStrategyMetrics(const std::string& name, double pnl, bool is_winning_trade);

    mutable boost::mutex strategy_mutex_;
    std::map<std::string, StrategyConfig> strategies_;
    std::map<std::string, StrategyMetrics> strategy_metrics_;
    std::function<void(const std::string&, const StrategyMetrics&)> strategy_callback_;
    std::function<void(const std::string&, double, double, const std::string&)> trade_callback_;
    const ConfigManager& config_manager_;
    const MarketDataManager& market_data_manager_;
    const RiskManager& risk_manager_;
};

#endif // STRATEGY_MANAGER_H 