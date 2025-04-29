#include "latency_module.h"
#include <algorithm>
#include <numeric>
#include <filesystem>
#include <stdexcept>
#include <fstream>
#include <iomanip>
#include <sstream>
#include <ctime>

LatencyModule::LatencyModule() {
    ensureLogFileOpen();
}

LatencyModule::~LatencyModule() {
    if (log_file_.is_open()) {
        log_file_.close();
    }
}

LatencyModule::TimePoint LatencyModule::start(const std::string& operation_id) {
    return std::chrono::steady_clock::now();
}

void LatencyModule::end(const std::string& operation_id, const TimePoint& start_time) {
    std::lock_guard<std::mutex> lock(mutex_);
    auto end_time = std::chrono::steady_clock::now();
    auto latency = std::chrono::duration_cast<Duration>(end_time - start_time);
    
    if (operation_id == "order_placement") {
        trackOrderPlacement(latency);
    } else if (operation_id == "market_data") {
        trackMarketData(latency);
    } else if (operation_id == "websocket") {
        trackWebSocketMessage(latency);
    } else if (operation_id == "trading_loop") {
        trackTradingLoop(latency);
    }
    
    latency_data_[operation_id].push_back(latency);
    if (latency_data_[operation_id].size() > max_history_size_) {
        latency_data_[operation_id].erase(latency_data_[operation_id].begin());
    }
}

void LatencyModule::trackOrderPlacement(const Duration& latency) {
    std::lock_guard<std::mutex> lock(mutex_);
    order_placement_latencies_.push_back(latency);
    if (order_placement_latencies_.size() > max_history_size_) {
        order_placement_latencies_.erase(order_placement_latencies_.begin());
    }
}

void LatencyModule::trackMarketData(const Duration& latency) {
    std::lock_guard<std::mutex> lock(mutex_);
    market_data_latencies_.push_back(latency);
    if (market_data_latencies_.size() > max_history_size_) {
        market_data_latencies_.erase(market_data_latencies_.begin());
    }
}

void LatencyModule::trackWebSocketMessage(const Duration& latency) {
    std::lock_guard<std::mutex> lock(mutex_);
    websocket_latencies_.push_back(latency);
    if (websocket_latencies_.size() > max_history_size_) {
        websocket_latencies_.erase(websocket_latencies_.begin());
    }
}

void LatencyModule::trackTradingLoop(const Duration& latency) {
    std::lock_guard<std::mutex> lock(mutex_);
    trading_loop_latencies_.push_back(latency);
    if (trading_loop_latencies_.size() > max_history_size_) {
        trading_loop_latencies_.erase(trading_loop_latencies_.begin());
    }
}

LatencyModule::LatencyStats LatencyModule::getOrderPlacementStats() const {
    std::lock_guard<std::mutex> lock(mutex_);
    LatencyStats stats;
    calculateStats(order_placement_latencies_, stats);
    return stats;
}

LatencyModule::LatencyStats LatencyModule::getMarketDataStats() const {
    std::lock_guard<std::mutex> lock(mutex_);
    LatencyStats stats;
    calculateStats(market_data_latencies_, stats);
    return stats;
}

LatencyModule::LatencyStats LatencyModule::getWebSocketStats() const {
    std::lock_guard<std::mutex> lock(mutex_);
    LatencyStats stats;
    calculateStats(websocket_latencies_, stats);
    return stats;
}

LatencyModule::LatencyStats LatencyModule::getTradingLoopStats() const {
    std::lock_guard<std::mutex> lock(mutex_);
    LatencyStats stats;
    calculateStats(trading_loop_latencies_, stats);
    return stats;
}

void LatencyModule::calculateStats(const std::vector<Duration>& latencies, LatencyStats& stats) const {
    if (latencies.empty()) {
        return;
    }

    stats.count = latencies.size();
    stats.timestamp = std::chrono::system_clock::now();

    // Calculate min/max
    auto [min_it, max_it] = std::minmax_element(latencies.begin(), latencies.end());
    stats.min = *min_it;
    stats.max = *max_it;

    // Calculate average
    auto sum = std::accumulate(latencies.begin(), latencies.end(), Duration::zero());
    stats.avg = Duration(sum.count() / latencies.size());

    // Calculate percentiles
    std::vector<Duration> sorted_latencies = latencies;
    std::sort(sorted_latencies.begin(), sorted_latencies.end());

    size_t p50_idx = (sorted_latencies.size() - 1) * 50 / 100;
    size_t p90_idx = (sorted_latencies.size() - 1) * 90 / 100;
    size_t p99_idx = (sorted_latencies.size() - 1) * 99 / 100;

    stats.p50 = sorted_latencies[p50_idx];
    stats.p90 = sorted_latencies[p90_idx];
    stats.p99 = sorted_latencies[p99_idx];
}

void LatencyModule::saveStats(const std::string& filename) const {
    std::lock_guard<std::mutex> lock(mutex_);
    std::ofstream file(filename);
    if (!file.is_open()) {
        return;
    }

    // Write header
    file << "Operation,Min (μs),Avg (μs),Max (μs),P50 (μs),P90 (μs),P99 (μs),Count\n";

    // Write stats for each operation type
    auto write_stats = [&file](const std::string& name, const LatencyStats& stats) {
        file << name << ","
             << stats.min.count() << ","
             << stats.avg.count() << ","
             << stats.max.count() << ","
             << stats.p50.count() << ","
             << stats.p90.count() << ","
             << stats.p99.count() << ","
             << stats.count << "\n";
    };

    write_stats("Order Placement", getOrderPlacementStats());
    write_stats("Market Data", getMarketDataStats());
    write_stats("WebSocket", getWebSocketStats());
    write_stats("Trading Loop", getTradingLoopStats());

    file.close();
}

void LatencyModule::resetStats() {
    std::lock_guard<std::mutex> lock(mutex_);
    order_placement_latencies_.clear();
    market_data_latencies_.clear();
    websocket_latencies_.clear();
    trading_loop_latencies_.clear();
    latency_data_.clear();
}

void LatencyModule::clearStats(const std::string& operation_id) {
    std::lock_guard<std::mutex> lock(mutex_);
    latency_data_[operation_id].clear();
}

void LatencyModule::clearAllStats() {
    resetStats();
}

void LatencyModule::setHistorySize(size_t size) {
    std::lock_guard<std::mutex> lock(mutex_);
    max_history_size_ = size;
}

void LatencyModule::log(const std::string& message) {
    std::lock_guard<std::mutex> lock(mutex_);
    ensureLogFileOpen();
    if (log_file_.is_open()) {
        auto now = std::chrono::system_clock::now();
        auto time_t = std::chrono::system_clock::to_time_t(now);
        std::tm tm;
        localtime_s(&tm, &time_t);
        
        log_file_ << std::put_time(&tm, "%Y-%m-%d %H:%M:%S") 
                  << " - " << message << std::endl;
    }
}

void LatencyModule::ensureLogFileOpen() {
    if (!log_file_.is_open()) {
        log_file_.open("latency.log", std::ios::app);
    }
}

LatencyModule::LatencyStats LatencyModule::getStats(const std::string& operation_id) const {
    std::lock_guard<std::mutex> lock(mutex_);
    LatencyStats stats;
    auto it = latency_data_.find(operation_id);
    if (it != latency_data_.end()) {
        calculateStats(it->second, stats);
    }
    return stats;
}

std::vector<LatencyModule::LatencyStats> LatencyModule::getHistoricalStats(
    const std::string& operation_id) const {
    std::lock_guard<std::mutex> lock(mutex_);
    std::vector<LatencyStats> result;
    auto it = latency_data_.find(operation_id);
    if (it != latency_data_.end()) {
        result.push_back(getStats(operation_id));
    }
    return result;
}
