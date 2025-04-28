#include "latency_module.h"
#include <algorithm>
#include <numeric>
#include <filesystem>

std::mutex LatencyModule::stats_mutex_;
std::map<std::string, std::vector<LatencyModule::Duration>> LatencyModule::latency_data_;
std::ofstream LatencyModule::log_file_;

LatencyModule::TimePoint LatencyModule::start() {
    return std::chrono::high_resolution_clock::now();
}

void LatencyModule::end(TimePoint start, const std::string& operation) {
    auto end = std::chrono::high_resolution_clock::now();
    auto duration = std::chrono::duration_cast<Duration>(end - start);
    
    std::lock_guard<std::mutex> lock(stats_mutex_);
    latency_data_[operation].push_back(duration);
    
    // Log the operation
    log(operation + ": " + std::to_string(duration.count()) + " ns");
}

void LatencyModule::log(const std::string& message) {
    ensureLogFileOpen();
    auto now = std::chrono::system_clock::now();
    auto time = std::chrono::system_clock::to_time_t(now);
    log_file_ << std::put_time(std::localtime(&time), "%Y-%m-%d %H:%M:%S") 
              << " - " << message << std::endl;
}

void LatencyModule::saveStats(const std::string& filename) {
    std::ofstream file(filename);
    if (!file.is_open()) {
        throw std::runtime_error("Failed to open file for writing stats");
    }
    
    std::lock_guard<std::mutex> lock(stats_mutex_);
    
    file << "Operation,Min(ns),Max(ns),Avg(ns),P50(ns),P90(ns),P99(ns),Count\n";
    
    for (const auto& [operation, data] : latency_data_) {
        LatencyStats stats;
        calculateStats(data, stats);
        
        file << operation << ","
             << stats.min.count() << ","
             << stats.max.count() << ","
             << stats.avg.count() << ","
             << stats.p50.count() << ","
             << stats.p90.count() << ","
             << stats.p99.count() << ","
             << stats.count << "\n";
    }
}

void LatencyModule::resetStats() {
    std::lock_guard<std::mutex> lock(stats_mutex_);
    latency_data_.clear();
}

void LatencyModule::trackOrderPlacement(const Duration& latency) {
    std::lock_guard<std::mutex> lock(stats_mutex_);
    latency_data_["order_placement"].push_back(latency);
}

void LatencyModule::trackMarketData(const Duration& latency) {
    std::lock_guard<std::mutex> lock(stats_mutex_);
    latency_data_["market_data"].push_back(latency);
}

void LatencyModule::trackWebSocketMessage(const Duration& latency) {
    std::lock_guard<std::mutex> lock(stats_mutex_);
    latency_data_["websocket_message"].push_back(latency);
}

void LatencyModule::trackTradingLoop(const Duration& latency) {
    std::lock_guard<std::mutex> lock(stats_mutex_);
    latency_data_["trading_loop"].push_back(latency);
}

LatencyModule::LatencyStats LatencyModule::getOrderPlacementStats() {
    std::lock_guard<std::mutex> lock(stats_mutex_);
    LatencyStats stats;
    calculateStats(latency_data_["order_placement"], stats);
    return stats;
}

LatencyModule::LatencyStats LatencyModule::getMarketDataStats() {
    std::lock_guard<std::mutex> lock(stats_mutex_);
    LatencyStats stats;
    calculateStats(latency_data_["market_data"], stats);
    return stats;
}

LatencyModule::LatencyStats LatencyModule::getWebSocketStats() {
    std::lock_guard<std::mutex> lock(stats_mutex_);
    LatencyStats stats;
    calculateStats(latency_data_["websocket_message"], stats);
    return stats;
}

LatencyModule::LatencyStats LatencyModule::getTradingLoopStats() {
    std::lock_guard<std::mutex> lock(stats_mutex_);
    LatencyStats stats;
    calculateStats(latency_data_["trading_loop"], stats);
    return stats;
}

void LatencyModule::calculateStats(const std::vector<Duration>& data, LatencyStats& stats) {
    if (data.empty()) {
        stats.min = Duration::zero();
        stats.max = Duration::zero();
        stats.avg = Duration::zero();
        stats.p50 = Duration::zero();
        stats.p90 = Duration::zero();
        stats.p99 = Duration::zero();
        stats.count = 0;
        return;
    }
    
    auto sorted_data = data;
    std::sort(sorted_data.begin(), sorted_data.end());
    
    stats.min = sorted_data.front();
    stats.max = sorted_data.back();
    stats.count = sorted_data.size();
    
    // Calculate average
    auto sum = std::accumulate(sorted_data.begin(), sorted_data.end(), Duration::zero());
    stats.avg = sum / sorted_data.size();
    
    // Calculate percentiles
    stats.p50 = sorted_data[sorted_data.size() * 50 / 100];
    stats.p90 = sorted_data[sorted_data.size() * 90 / 100];
    stats.p99 = sorted_data[sorted_data.size() * 99 / 100];
}

void LatencyModule::ensureLogFileOpen() {
    if (!log_file_.is_open()) {
        std::filesystem::create_directories("logs");
        auto now = std::chrono::system_clock::now();
        auto time = std::chrono::system_clock::to_time_t(now);
        std::stringstream ss;
        ss << "logs/latency_" << std::put_time(std::localtime(&time), "%Y%m%d_%H%M%S") << ".log";
        log_file_.open(ss.str(), std::ios::app);
    }
}
