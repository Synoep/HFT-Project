#pragma once
#ifndef LATENCY_MODULE_H
#define LATENCY_MODULE_H

#include <string>
#include <vector>
#include <map>
#include <chrono>
#include <mutex>
#include <fstream>

class LatencyModule {
public:
    using Duration = std::chrono::microseconds;
    using TimePoint = std::chrono::steady_clock::time_point;

    struct LatencyStats {
        Duration min;
        Duration max;
        Duration avg;
        Duration p50;
        Duration p90;
        Duration p99;
        size_t count;
        std::chrono::system_clock::time_point timestamp;
    };

    static LatencyModule& getInstance() {
        static LatencyModule instance;
        return instance;
    }

    TimePoint start(const std::string& operation_id);
    void end(const std::string& operation_id, const TimePoint& start_time);
    
    void trackOrderPlacement(const Duration& latency);
    void trackMarketData(const Duration& latency);
    void trackWebSocketMessage(const Duration& latency);
    void trackTradingLoop(const Duration& latency);

    LatencyStats getOrderPlacementStats() const;
    LatencyStats getMarketDataStats() const;
    LatencyStats getWebSocketStats() const;
    LatencyStats getTradingLoopStats() const;
    LatencyStats getStats(const std::string& operation_id) const;
    std::vector<LatencyStats> getHistoricalStats(const std::string& operation_id) const;

    void saveStats(const std::string& filename) const;
    void resetStats();
    void clearStats(const std::string& operation_id);
    void clearAllStats();
    void setHistorySize(size_t size);
    void log(const std::string& message);

private:
    LatencyModule();
    ~LatencyModule();
    LatencyModule(const LatencyModule&) = delete;
    LatencyModule& operator=(const LatencyModule&) = delete;

    void calculateStats(const std::vector<Duration>& latencies, LatencyStats& stats) const;
    void ensureLogFileOpen();

    mutable std::mutex mutex_;
    std::ofstream log_file_;
    size_t max_history_size_ = 1000;
    std::map<std::string, std::vector<Duration>> latency_data_;
    std::vector<Duration> order_placement_latencies_;
    std::vector<Duration> market_data_latencies_;
    std::vector<Duration> websocket_latencies_;
    std::vector<Duration> trading_loop_latencies_;
};

#endif // LATENCY_MODULE_H
