#pragma once
#ifndef LATENCY_MODULE_H
#define LATENCY_MODULE_H

#include <chrono>
#include <string>
#include <vector>
#include <mutex>
#include <fstream>
#include <atomic>
#include <map>

class LatencyModule {
public:
    using TimePoint = std::chrono::high_resolution_clock::time_point;
    using Duration = std::chrono::nanoseconds;
    
    struct LatencyStats {
        Duration min_latency;
        Duration max_latency;
        Duration avg_latency;
        Duration p95_latency;
        Duration p99_latency;
        size_t total_operations;
        size_t error_count;
    };
    
    static TimePoint start() {
        return std::chrono::high_resolution_clock::now();
    }
    
    static void trackWebSocketMessage(const Duration& latency) {
        std::lock_guard<std::mutex> lock(stats_mutex_);
        webSocketLatencies_.push_back(latency);
        updateStats(webSocketLatencies_, webSocketStats_);
    }
    
    static void trackOrderPlacement(const Duration& latency) {
        std::lock_guard<std::mutex> lock(stats_mutex_);
        orderLatencies_.push_back(latency);
        updateStats(orderLatencies_, orderStats_);
    }
    
    static void trackMarketData(const Duration& latency) {
        std::lock_guard<std::mutex> lock(stats_mutex_);
        marketDataLatencies_.push_back(latency);
        updateStats(marketDataLatencies_, marketDataStats_);
    }
    
    static void trackError() {
        std::lock_guard<std::mutex> lock(stats_mutex_);
        errorCount_++;
    }
    
    static void saveStatsToFile() {
        std::lock_guard<std::mutex> lock(stats_mutex_);
        std::ofstream file("performance_stats.csv", std::ios::app);
        
        auto now = std::chrono::system_clock::now();
        auto time = std::chrono::system_clock::to_time_t(now);
        
        file << "\nTimestamp: " << std::ctime(&time);
        file << "Metric,Min,Max,Average,P95,P99,Total Operations,Errors\n";
        
        saveStatsToFile(file, "WebSocket", webSocketStats_);
        saveStatsToFile(file, "Order Placement", orderStats_);
        saveStatsToFile(file, "Market Data", marketDataStats_);
        
        file.close();
    }
    
    static const LatencyStats& getWebSocketStats() { return webSocketStats_; }
    static const LatencyStats& getOrderStats() { return orderStats_; }
    static const LatencyStats& getMarketDataStats() { return marketDataStats_; }
    static size_t getErrorCount() { return errorCount_; }

private:
    static void updateStats(std::vector<Duration>& latencies, LatencyStats& stats) {
        if (latencies.empty()) return;
        
        std::sort(latencies.begin(), latencies.end());
        
        stats.min_latency = latencies.front();
        stats.max_latency = latencies.back();
        
        Duration sum = Duration::zero();
        for (const auto& latency : latencies) {
            sum += latency;
        }
        stats.avg_latency = sum / latencies.size();
        
        size_t p95_index = static_cast<size_t>(latencies.size() * 0.95);
        size_t p99_index = static_cast<size_t>(latencies.size() * 0.99);
        
        stats.p95_latency = latencies[p95_index];
        stats.p99_latency = latencies[p99_index];
        
        stats.total_operations = latencies.size();
        stats.error_count = errorCount_;
    }
    
    static void saveStatsToFile(std::ofstream& file, const std::string& name, const LatencyStats& stats) {
        file << name << ","
             << stats.min_latency.count() << ","
             << stats.max_latency.count() << ","
             << stats.avg_latency.count() << ","
             << stats.p95_latency.count() << ","
             << stats.p99_latency.count() << ","
             << stats.total_operations << ","
             << stats.error_count << "\n";
    }
    
    static std::mutex stats_mutex_;
    static std::vector<Duration> webSocketLatencies_;
    static std::vector<Duration> orderLatencies_;
    static std::vector<Duration> marketDataLatencies_;
    static LatencyStats webSocketStats_;
    static LatencyStats orderStats_;
    static LatencyStats marketDataStats_;
    static std::atomic<size_t> errorCount_;
};

#endif // LATENCY_MODULE_H
