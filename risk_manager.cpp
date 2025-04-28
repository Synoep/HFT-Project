#include "risk_manager.h"
#include <algorithm>
#include <cmath>
#include <stdexcept>

RiskManager::RiskManager()
    : config_manager_(ConfigManager::getInstance()),
      market_data_manager_(MarketDataManager::getInstance()) {
    risk_metrics_ = {
        0.0,    // total_exposure
        0.0,    // daily_pnl
        0.0,    // max_drawdown
        0.0,    // sharpe_ratio
        0.0,    // win_rate
        0,      // total_trades
        0,      // winning_trades
        std::chrono::system_clock::now()  // timestamp
    };
}

RiskManager::~RiskManager() {
    shutdown();
}

void RiskManager::initialize() {
    // Initialize risk metrics
    risk_metrics_ = {
        0.0,    // total_exposure
        0.0,    // daily_pnl
        0.0,    // max_drawdown
        0.0,    // sharpe_ratio
        0.0,    // win_rate
        0,      // total_trades
        0,      // winning_trades
        std::chrono::system_clock::now()  // timestamp
    };
}

void RiskManager::shutdown() {
    // Clean up resources if needed
}

bool RiskManager::checkOrderRisk(const std::string& instrument, double size, double price, const std::string& side) {
    std::lock_guard<std::mutex> lock(risk_mutex_);
    
    // Check position limit
    if (!checkPositionLimit(instrument, size)) {
        notifyRiskViolation(instrument, "Position limit exceeded");
        return false;
    }
    
    // Calculate potential loss
    double potential_loss = 0.0;
    if (side == "buy") {
        potential_loss = size * price;
    } else if (side == "sell") {
        potential_loss = size * price;
    }
    
    // Check loss limits
    if (!checkLossLimit(potential_loss)) {
        notifyRiskViolation(instrument, "Loss limit exceeded");
        return false;
    }
    
    if (!checkDailyLossLimit(potential_loss)) {
        notifyRiskViolation(instrument, "Daily loss limit exceeded");
        return false;
    }
    
    // Check exposure limit
    double new_exposure = risk_metrics_.total_exposure + potential_loss;
    if (!checkExposureLimit(new_exposure)) {
        notifyRiskViolation(instrument, "Exposure limit exceeded");
        return false;
    }
    
    return true;
}

void RiskManager::updatePosition(const Position& position) {
    std::lock_guard<std::mutex> lock(risk_mutex_);
    positions_[position.instrument] = position;
    
    // Update total exposure
    double total_exposure = 0.0;
    for (const auto& [_, pos] : positions_) {
        total_exposure += std::abs(pos.size * pos.avg_price);
    }
    risk_metrics_.total_exposure = total_exposure;
    
    // Notify position update
    if (position_callback_) {
        position_callback_(position);
    }
}

void RiskManager::updateRiskMetrics(const RiskMetrics& metrics) {
    std::lock_guard<std::mutex> lock(risk_mutex_);
    risk_metrics_ = metrics;
    
    // Update win rate
    if (metrics.total_trades > 0) {
        risk_metrics_.win_rate = static_cast<double>(metrics.winning_trades) / metrics.total_trades;
    }
    
    // Notify metrics update
    if (metrics_callback_) {
        metrics_callback_(metrics);
    }
}

const RiskManager::Position& RiskManager::getPosition(const std::string& instrument) const {
    std::lock_guard<std::mutex> lock(risk_mutex_);
    auto it = positions_.find(instrument);
    if (it == positions_.end()) {
        throw std::runtime_error("No position found for instrument: " + instrument);
    }
    return it->second;
}

const RiskManager::RiskMetrics& RiskManager::getRiskMetrics() const {
    std::lock_guard<std::mutex> lock(risk_mutex_);
    return risk_metrics_;
}

double RiskManager::getTotalExposure() const {
    std::lock_guard<std::mutex> lock(risk_mutex_);
    return risk_metrics_.total_exposure;
}

double RiskManager::getDailyPnL() const {
    std::lock_guard<std::mutex> lock(risk_mutex_);
    return risk_metrics_.daily_pnl;
}

double RiskManager::getMaxDrawdown() const {
    std::lock_guard<std::mutex> lock(risk_mutex_);
    return risk_metrics_.max_drawdown;
}

void RiskManager::setRiskCallback(std::function<void(const std::string&, const std::string&)> callback) {
    std::lock_guard<std::mutex> lock(risk_mutex_);
    risk_callback_ = callback;
}

void RiskManager::setPositionCallback(std::function<void(const Position&)> callback) {
    std::lock_guard<std::mutex> lock(risk_mutex_);
    position_callback_ = callback;
}

void RiskManager::setMetricsCallback(std::function<void(const RiskMetrics&)> callback) {
    std::lock_guard<std::mutex> lock(risk_mutex_);
    metrics_callback_ = callback;
}

bool RiskManager::checkPositionLimit(const std::string& instrument, double size) {
    const auto& trading_config = config_manager_.getTradingConfig();
    
    // Check if position size exceeds maximum
    if (std::abs(size) > trading_config.max_position_size) {
        return false;
    }
    
    // Check if order size exceeds maximum
    if (std::abs(size) > trading_config.max_order_size) {
        return false;
    }
    
    return true;
}

bool RiskManager::checkLossLimit(double potential_loss) {
    const auto& trading_config = config_manager_.getTradingConfig();
    return potential_loss <= trading_config.max_loss_per_trade;
}

bool RiskManager::checkDailyLossLimit(double potential_loss) {
    const auto& trading_config = config_manager_.getTradingConfig();
    return (risk_metrics_.daily_pnl - potential_loss) >= -trading_config.max_daily_loss;
}

bool RiskManager::checkExposureLimit(double exposure) {
    const auto& trading_config = config_manager_.getTradingConfig();
    return exposure <= trading_config.max_position_size;
}

void RiskManager::notifyRiskViolation(const std::string& instrument, const std::string& reason) {
    if (risk_callback_) {
        risk_callback_(instrument, reason);
    }
} 