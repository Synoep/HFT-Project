#ifndef RISK_MANAGER_H
#define RISK_MANAGER_H

#include <string>
#include <map>
#include <mutex>
#include <memory>
#include <functional>
#include <chrono>
#include "config_manager.h"
#include "market_data_manager.h"

class RiskManager {
public:
    struct Position {
        std::string instrument;
        double size;
        double avg_price;
        double unrealized_pnl;
        double realized_pnl;
        std::chrono::system_clock::time_point timestamp;
    };

    struct RiskMetrics {
        double total_exposure;
        double daily_pnl;
        double max_drawdown;
        double sharpe_ratio;
        double win_rate;
        int total_trades;
        int winning_trades;
        std::chrono::system_clock::time_point timestamp;
    };

    static RiskManager& getInstance() {
        static RiskManager instance;
        return instance;
    }

    void initialize();
    void shutdown();

    bool checkOrderRisk(const std::string& instrument, double size, double price, const std::string& side);
    void updatePosition(const Position& position);
    void updateRiskMetrics(const RiskMetrics& metrics);

    const Position& getPosition(const std::string& instrument) const;
    const RiskMetrics& getRiskMetrics() const;
    double getTotalExposure() const;
    double getDailyPnL() const;
    double getMaxDrawdown() const;

    void setRiskCallback(std::function<void(const std::string&, const std::string&)> callback);
    void setPositionCallback(std::function<void(const Position&)> callback);
    void setMetricsCallback(std::function<void(const RiskMetrics&)> callback);

private:
    RiskManager();
    ~RiskManager();
    RiskManager(const RiskManager&) = delete;
    RiskManager& operator=(const RiskManager&) = delete;

    bool checkPositionLimit(const std::string& instrument, double size);
    bool checkLossLimit(double potential_loss);
    bool checkDailyLossLimit(double potential_loss);
    bool checkExposureLimit(double exposure);
    void notifyRiskViolation(const std::string& instrument, const std::string& reason);

    mutable std::mutex risk_mutex_;
    std::map<std::string, Position> positions_;
    RiskMetrics risk_metrics_;
    std::function<void(const std::string&, const std::string&)> risk_callback_;
    std::function<void(const Position&)> position_callback_;
    std::function<void(const RiskMetrics&)> metrics_callback_;
    const ConfigManager& config_manager_;
    const MarketDataManager& market_data_manager_;
};

#endif // RISK_MANAGER_H 