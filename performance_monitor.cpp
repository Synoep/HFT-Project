#include "performance_monitor.h"
#include <algorithm>
#include <numeric>
#include <filesystem>
#include <sstream>
#include <iomanip>
#include <ctime>

PerformanceMonitor::PerformanceMonitor()
    : detailed_tracking_enabled_(true),
      sampling_interval_(std::chrono::milliseconds(100)),
      total_memory_usage_(0),
      total_cpu_usage_(0) {
    std::filesystem::create_directory("performance_logs");
}

PerformanceMonitor::~PerformanceMonitor() {
    saveStatsToFile();
}

void PerformanceMonitor::startOperation(const std::string& operation_name) {
    std::lock_guard<std::mutex> lock(stats_mutex_);
    OperationMetrics metrics;
    metrics.start_time = std::chrono::high_resolution_clock::now();
    operation_metrics_[operation_name].push_back(metrics);
}

void PerformanceMonitor::endOperation(const std::string& operation_name, bool success) {
    std::lock_guard<std::mutex> lock(stats_mutex_);
    auto& metrics_list = operation_metrics_[operation_name];
    if (metrics_list.empty()) return;
    
    auto& metrics = metrics_list.back();
    metrics.latency = std::chrono::duration_cast<Duration>(
        std::chrono::high_resolution_clock::now() - metrics.start_time);
    metrics.memory_used = total_memory_usage_;
    metrics.cpu_used = total_cpu_usage_;
    metrics.success = success;
    
    updateStats(operation_name, metrics);
    
    if (detailed_tracking_enabled_) {
        for (const auto& callback : callbacks_) {
            callback(operation_name, metrics);
        }
    }
}

void PerformanceMonitor::trackMemoryUsage(size_t bytes) {
    total_memory_usage_ = bytes;
}

void PerformanceMonitor::trackCPUUsage(size_t percentage) {
    total_cpu_usage_ = percentage;
}

void PerformanceMonitor::updateStats(const std::string& operation_name, const OperationMetrics& metrics) {
    auto& stats = operation_stats_[operation_name];
    auto& metrics_list = operation_metrics_[operation_name];
    
    if (metrics_list.empty()) return;
    
    // Calculate statistics
    std::vector<Duration> latencies;
    latencies.reserve(metrics_list.size());
    for (const auto& m : metrics_list) {
        latencies.push_back(m.latency);
    }
    
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
    
    stats.total_operations = metrics_list.size();
    stats.error_count = std::count_if(metrics_list.begin(), metrics_list.end(),
                                    [](const OperationMetrics& m) { return !m.success; });
    stats.memory_usage = metrics.memory_used;
    stats.cpu_usage = metrics.cpu_used;
}

void PerformanceMonitor::saveStatsToFile() {
    std::lock_guard<std::mutex> lock(stats_mutex_);
    auto now = std::chrono::system_clock::now();
    auto time = std::chrono::system_clock::to_time_t(now);
    std::stringstream filename;
    filename << "performance_logs/stats_" << std::put_time(std::localtime(&time), "%Y%m%d_%H%M%S") << ".csv";
    
    std::ofstream file(filename.str());
    file << "Operation,Min Latency (ns),Max Latency (ns),Avg Latency (ns),"
         << "P95 Latency (ns),P99 Latency (ns),Total Operations,Errors,"
         << "Memory Usage (bytes),CPU Usage (%)\n";
    
    for (const auto& [name, stats] : operation_stats_) {
        saveStatsToFile(file, name, stats);
    }
    
    file.close();
}

void PerformanceMonitor::saveStatsToFile(std::ofstream& file, const std::string& name, const PerformanceStats& stats) {
    file << name << ","
         << stats.min_latency.count() << ","
         << stats.max_latency.count() << ","
         << stats.avg_latency.count() << ","
         << stats.p95_latency.count() << ","
         << stats.p99_latency.count() << ","
         << stats.total_operations << ","
         << stats.error_count << ","
         << stats.memory_usage << ","
         << stats.cpu_usage << "\n";
}

const PerformanceMonitor::PerformanceStats& PerformanceMonitor::getStats(const std::string& operation_name) {
    std::lock_guard<std::mutex> lock(stats_mutex_);
    return operation_stats_[operation_name];
}

void PerformanceMonitor::setMetricsCallback(std::function<void(const std::string&, const OperationMetrics&)> callback) {
    std::lock_guard<std::mutex> lock(stats_mutex_);
    callbacks_.push_back(callback);
}

void PerformanceMonitor::enableDetailedTracking(bool enable) {
    detailed_tracking_enabled_ = enable;
}

void PerformanceMonitor::setSamplingInterval(Duration interval) {
    sampling_interval_ = interval;
} 