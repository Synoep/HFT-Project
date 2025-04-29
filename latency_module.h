#pragma once
#ifndef LATENCY_MODULE_H
#define LATENCY_MODULE_H

#include <string>
#include <chrono>
#include <map>
#include <vector>
#include <mutex>
#include <memory>
#include <fstream>

class LatencyModule {
public:
    using Duration = std::chrono::microseconds;
    using TimePoint = std::chrono::steady_clock::time_point;

    struct LatencyStats {
        Duration min{Duration::zero()};
        Duration max{Duration::zero()};
        Duration avg{Duration::zero()};
        Duration p50{Duration::zero()};
        Duration p90{Duration::zero()};
        Duration p99{Duration::zero()};
        size_t count{0};
        std::chrono::system_clock::time_point timestamp;
    };

    static LatencyModule& getInstance() {
        static LatencyModule instance;
        return instance;
    }

    // Core measurement functions
    TimePoint start(const std::string& operation_id);
    void end(const std::string& operation_id, const TimePoint& start_time);
    
    // Specific tracking functions
    void trackOrderPlacement(const Duration& latency);
    void trackMarketData(const Duration& latency);
    void trackWebSocketMessage(const Duration& latency);
    void trackTradingLoop(const Duration& latency);

    // Stats retrieval functions
    LatencyStats getOrderPlacementStats() const;
    LatencyStats getMarketDataStats() const;
    LatencyStats getWebSocketStats() const;
    LatencyStats getTradingLoopStats() const;

    // Utility functions
    void saveStats(const std::string& filename) const;
    void resetStats();
    void log(const std::string& message);

private:
    LatencyModule();
    ~LatencyModule();
    LatencyModule(const LatencyModule&) = delete;
    LatencyModule& operator=(const LatencyModule&) = delete;

    void calculateStats(const std::vector<Duration>& latencies, LatencyStats& stats) const;
    void ensureLogFileOpen();

    mutable std::mutex mutex_;
    std::map<std::string, std::vector<Duration>> latency_data_;
    std::ofstream log_file_;

    // Operation-specific latency storage
    std::vector<Duration> order_placement_latencies_;
    std::vector<Duration> market_data_latencies_;
    std::vector<Duration> websocket_latencies_;
    std::vector<Duration> trading_loop_latencies_;

    size_t max_history_size_{1000};
};

#endif // LATENCY_MODULE_H
