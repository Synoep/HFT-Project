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
    auto end_time = std::chrono::steady_clock::now();
    auto latency = std::chrono::duration_cast<Duration>(end_time - start_time);
    
    std::lock_guard<std::mutex> lock(mutex_);
    auto& latencies = latency_data_[operation_id];
    latencies.push_back(latency);
    
    if (latencies.size() > max_history_size_) {
        latencies.erase(latencies.begin());
    }
}

void LatencyModule::log(const std::string& message) {
    std::lock_guard<std::mutex> lock(mutex_);
    ensureLogFileOpen();
    if (log_file_.is_open()) {
        auto now = std::chrono::system_clock::now();
        auto time_t = std::chrono::system_clock::to_time_t(now);
        log_file_ << std::put_time(std::localtime(&time_t), "%Y-%m-%d %H:%M:%S") 
                 << " - " << message << std::endl;
    }
}

void LatencyModule::saveStats(const std::string& filename) const {
    std::ofstream file(filename);
    if (!file.is_open()) {
        return;
    }

    std::lock_guard<std::mutex> lock(mutex_);
    file << "Operation,Count,Min (μs),Max (μs),Avg (μs),P50 (μs),P90 (μs),P99 (μs)\n";

    for (const auto& [operation, latencies] : latency_data_) {
        LatencyStats stats;
        calculateStats(latencies, stats);
        file << operation << ","
             << stats.count << ","
             << stats.min.count() << ","
             << stats.max.count() << ","
             << stats.avg.count() << ","
             << stats.p50.count() << ","
             << stats.p90.count() << ","
             << stats.p99.count() << "\n";
    }
}

void LatencyModule::resetStats() {
    std::lock_guard<std::mutex> lock(mutex_);
    latency_data_.clear();
}

LatencyModule::LatencyStats LatencyModule::getOrderPlacementStats() const {
    std::lock_guard<std::mutex> lock(mutex_);
    LatencyStats stats;
    auto it = latency_data_.find("order_placement");
    if (it != latency_data_.end()) {
        calculateStats(it->second, stats);
    }
    return stats;
}

LatencyModule::LatencyStats LatencyModule::getMarketDataStats() const {
    std::lock_guard<std::mutex> lock(mutex_);
    LatencyStats stats;
    auto it = latency_data_.find("market_data");
    if (it != latency_data_.end()) {
        calculateStats(it->second, stats);
    }
    return stats;
}

LatencyModule::LatencyStats LatencyModule::getWebSocketStats() const {
    std::lock_guard<std::mutex> lock(mutex_);
    LatencyStats stats;
    auto it = latency_data_.find("websocket");
    if (it != latency_data_.end()) {
        calculateStats(it->second, stats);
    }
    return stats;
}

LatencyModule::LatencyStats LatencyModule::getTradingLoopStats() const {
    std::lock_guard<std::mutex> lock(mutex_);
    LatencyStats stats;
    auto it = latency_data_.find("trading_loop");
    if (it != latency_data_.end()) {
        calculateStats(it->second, stats);
    }
    return stats;
}

void LatencyModule::calculateStats(const std::vector<Duration>& latencies, LatencyStats& stats) const {
    if (latencies.empty()) {
        return;
    }

    stats.count = latencies.size();
    stats.timestamp = std::chrono::system_clock::now();

    // Calculate min and max
    auto [min_it, max_it] = std::minmax_element(latencies.begin(), latencies.end());
    stats.min = *min_it;
    stats.max = *max_it;

    // Calculate average
    auto total = std::accumulate(latencies.begin(), latencies.end(), Duration::zero());
    stats.avg = Duration(total / latencies.size());

    // Sort for percentiles
    std::vector<Duration> sorted_latencies = latencies;
    std::sort(sorted_latencies.begin(), sorted_latencies.end());

    // Calculate percentiles
    size_t p50_idx = (sorted_latencies.size() - 1) * 50 / 100;
    size_t p90_idx = (sorted_latencies.size() - 1) * 90 / 100;
    size_t p99_idx = (sorted_latencies.size() - 1) * 99 / 100;

    stats.p50 = sorted_latencies[p50_idx];
    stats.p90 = sorted_latencies[p90_idx];
    stats.p99 = sorted_latencies[p99_idx];
}

void LatencyModule::ensureLogFileOpen() {
    if (!log_file_.is_open()) {
        auto now = std::chrono::system_clock::now();
        auto time_t = std::chrono::system_clock::to_time_t(now);
        std::stringstream ss;
        ss << "latency_log_" << std::put_time(std::localtime(&time_t), "%Y%m%d_%H%M%S") << ".txt";
        log_file_.open(ss.str(), std::ios::app);
    }
}

void LatencyModule::trackOrderPlacement(const Duration& latency) {
    std::lock_guard<std::mutex> lock(mutex_);
    auto& latencies = latency_data_["order_placement"];
    latencies.push_back(latency);
    if (latencies.size() > max_history_size_) {
        latencies.erase(latencies.begin());
    }
}

void LatencyModule::trackMarketData(const Duration& latency) {
    std::lock_guard<std::mutex> lock(mutex_);
    auto& latencies = latency_data_["market_data"];
    latencies.push_back(latency);
    if (latencies.size() > max_history_size_) {
        latencies.erase(latencies.begin());
    }
}

void LatencyModule::trackWebSocketMessage(const Duration& latency) {
    std::lock_guard<std::mutex> lock(mutex_);
    auto& latencies = latency_data_["websocket"];
    latencies.push_back(latency);
    if (latencies.size() > max_history_size_) {
        latencies.erase(latencies.begin());
    }
}

void LatencyModule::trackTradingLoop(const Duration& latency) {
    std::lock_guard<std::mutex> lock(mutex_);
    auto& latencies = latency_data_["trading_loop"];
    latencies.push_back(latency);
    if (latencies.size() > max_history_size_) {
        latencies.erase(latencies.begin());
    }
}

LatencyModule::LatencyStats LatencyModule::getStats(const std::string& operation_id) const {
    std::lock_guard<std::mutex> lock(mutex_);
    auto it = latency_data_.find(operation_id);
    if (it == latency_data_.end()) {
        throw std::runtime_error("Operation ID not found: " + operation_id);
    }
    LatencyStats stats;
    calculateStats(it->second, stats);
    return stats;
}

std::vector<LatencyModule::LatencyStats> LatencyModule::getHistoricalStats(
    const std::string& operation_id) const {
    std::lock_guard<std::mutex> lock(mutex_);
    auto it = latency_data_.find(operation_id);
    if (it == latency_data_.end()) {
        throw std::runtime_error("Operation ID not found: " + operation_id);
    }
    return {getStats(operation_id)};
}

void LatencyModule::clearStats(const std::string& operation_id) {
    std::lock_guard<std::mutex> lock(mutex_);
    auto it = latency_data_.find(operation_id);
    if (it != latency_data_.end()) {
        it->second.clear();
    }
}

void LatencyModule::clearAllStats() {
    std::lock_guard<std::mutex> lock(mutex_);
    latency_data_.clear();
}

void LatencyModule::setHistorySize(size_t size) {
    std::lock_guard<std::mutex> lock(mutex_);
    max_history_size_ = size;
    for (auto& [_, data] : latency_data_) {
        if (data.size() > max_history_size_) {
            data.resize(max_history_size_);
        }
    }
}
