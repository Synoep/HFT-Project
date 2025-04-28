#ifndef PERFORMANCE_MONITOR_H
#define PERFORMANCE_MONITOR_H

#include <chrono>
#include <string>
#include <vector>
#include <mutex>
#include <fstream>
#include <atomic>
#include <map>
#include <functional>

class PerformanceMonitor {
public:
    using TimePoint = std::chrono::high_resolution_clock::time_point;
    using Duration = std::chrono::nanoseconds;
    
    struct PerformanceStats {
        Duration min_latency;
        Duration max_latency;
        Duration avg_latency;
        Duration p95_latency;
        Duration p99_latency;
        size_t total_operations;
        size_t error_count;
        size_t memory_usage;
        size_t cpu_usage;
    };
    
    struct OperationMetrics {
        TimePoint start_time;
        Duration latency;
        size_t memory_used;
        size_t cpu_used;
        bool success;
    };
    
    static PerformanceMonitor& getInstance() {
        static PerformanceMonitor instance;
        return instance;
    }
    
    void startOperation(const std::string& operation_name);
    void endOperation(const std::string& operation_name, bool success = true);
    void trackMemoryUsage(size_t bytes);
    void trackCPUUsage(size_t percentage);
    void saveStatsToFile();
    const PerformanceStats& getStats(const std::string& operation_name);
    
    void setMetricsCallback(std::function<void(const std::string&, const OperationMetrics&)> callback);
    void enableDetailedTracking(bool enable);
    void setSamplingInterval(Duration interval);

private:
    PerformanceMonitor();
    ~PerformanceMonitor();
    PerformanceMonitor(const PerformanceMonitor&) = delete;
    PerformanceMonitor& operator=(const PerformanceMonitor&) = delete;
    
    void updateStats(const std::string& operation_name, const OperationMetrics& metrics);
    void saveStatsToFile(std::ofstream& file, const std::string& name, const PerformanceStats& stats);
    
    std::mutex stats_mutex_;
    std::map<std::string, std::vector<OperationMetrics>> operation_metrics_;
    std::map<std::string, PerformanceStats> operation_stats_;
    std::vector<std::function<void(const std::string&, const OperationMetrics&)>> callbacks_;
    bool detailed_tracking_enabled_;
    Duration sampling_interval_;
    std::atomic<size_t> total_memory_usage_;
    std::atomic<size_t> total_cpu_usage_;
};

#define START_OPERATION(name) PerformanceMonitor::getInstance().startOperation(name)
#define END_OPERATION(name, success) PerformanceMonitor::getInstance().endOperation(name, success)
#define TRACK_MEMORY(bytes) PerformanceMonitor::getInstance().trackMemoryUsage(bytes)
#define TRACK_CPU(percentage) PerformanceMonitor::getInstance().trackCPUUsage(percentage)

#endif // PERFORMANCE_MONITOR_H 