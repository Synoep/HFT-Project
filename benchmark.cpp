#include "benchmark.h"
#include <algorithm>
#include <numeric>
#include <thread>
#include <chrono>
#include <fstream>
#include <sstream>
#include <iomanip>
#include <filesystem>
#include <sysinfoapi.h>
#include <psapi.h>

Benchmark::Benchmark() = default;

Benchmark::~Benchmark() {
    stopResourceMonitoring();
}

void Benchmark::startOperation(const std::string& name) {
    std::lock_guard<std::mutex> lock(metrics_mutex_);
    auto& operation = operations_[name];
    operation.start_time = std::chrono::steady_clock::now();
    
    // Also start measurement in latency module
    latency_module_.startMeasurement(name);
}

void Benchmark::endOperation(const std::string& name, bool success) {
    std::lock_guard<std::mutex> lock(metrics_mutex_);
    auto it = operations_.find(name);
    if (it != operations_.end()) {
        auto end_time = std::chrono::steady_clock::now();
        auto duration = std::chrono::duration_cast<std::chrono::microseconds>(
            end_time - it->second.start_time).count() / 1000.0; // Convert to milliseconds
        
        it->second.latencies.push_back(duration);
        if (success) {
            it->second.success_count++;
        } else {
            it->second.error_count++;
        }
        
        updateMetrics(name);
    }
    
    // Also end measurement in latency module
    latency_module_.endMeasurement(name);
}

void Benchmark::recordError(const std::string& operation, const std::string& error_message) {
    std::lock_guard<std::mutex> lock(metrics_mutex_);
    auto it = operations_.find(operation);
    if (it != operations_.end()) {
        it->second.error_count++;
    }
}

OperationMetrics Benchmark::getMetrics(const std::string& operation) const {
    std::lock_guard<std::mutex> lock(metrics_mutex_);
    OperationMetrics metrics;
    metrics.operation_name = operation;
    
    auto it = operations_.find(operation);
    if (it != operations_.end()) {
        const auto& op_data = it->second;
        if (!op_data.latencies.empty()) {
            std::vector<double> sorted_latencies = op_data.latencies;
            std::sort(sorted_latencies.begin(), sorted_latencies.end());
            
            metrics.min_latency_ms = sorted_latencies.front();
            metrics.max_latency_ms = sorted_latencies.back();
            metrics.average_latency_ms = std::accumulate(sorted_latencies.begin(), 
                                                       sorted_latencies.end(), 0.0) 
                                       / sorted_latencies.size();
            
            size_t p95_idx = static_cast<size_t>(sorted_latencies.size() * 0.95);
            size_t p99_idx = static_cast<size_t>(sorted_latencies.size() * 0.99);
            metrics.p95_latency_ms = sorted_latencies[p95_idx];
            metrics.p99_latency_ms = sorted_latencies[p99_idx];
        }
        
        metrics.success_count = op_data.success_count;
        metrics.error_count = op_data.error_count;
        
        // Get current resource metrics
        auto resource_metrics = getCurrentResourceMetrics();
        metrics.cpu_usage = resource_metrics.cpu_usage_percent;
        metrics.memory_usage_mb = resource_metrics.memory_usage_mb;
    }
    
    metrics.timestamp = std::chrono::system_clock::now();
    return metrics;
}

std::vector<OperationMetrics> Benchmark::getAllMetrics() const {
    std::lock_guard<std::mutex> lock(metrics_mutex_);
    std::vector<OperationMetrics> all_metrics;
    
    for (const auto& [operation, _] : operations_) {
        all_metrics.push_back(getMetrics(operation));
    }
    
    return all_metrics;
}

void Benchmark::reset() {
    std::lock_guard<std::mutex> lock(metrics_mutex_);
    operations_.clear();
}

void Benchmark::enableResourceMonitoring(bool enable) {
    if (enable) {
        startResourceMonitoring();
    } else {
        stopResourceMonitoring();
    }
}

void Benchmark::startResourceMonitoring() {
    if (resource_monitoring_active_) return;
    
    resource_monitoring_active_ = true;
    stop_monitoring_ = false;
    
    resource_monitoring_thread_ = std::thread([this]() {
        while (!stop_monitoring_) {
            updateResourceMetrics();
            std::this_thread::sleep_for(sampling_interval_);
        }
    });
}

void Benchmark::stopResourceMonitoring() {
    if (!resource_monitoring_active_) return;
    
    stop_monitoring_ = true;
    if (resource_monitoring_thread_.joinable()) {
        resource_monitoring_thread_.join();
    }
    resource_monitoring_active_ = false;
}

void Benchmark::updateResourceMetrics() {
    ResourceMetrics metrics;
    
    // Get CPU usage
    FILETIME idle_time, kernel_time, user_time;
    if (GetSystemTimes(&idle_time, &kernel_time, &user_time)) {
        ULARGE_INTEGER kernel, user, idle;
        kernel.LowPart = kernel_time.dwLowDateTime;
        kernel.HighPart = kernel_time.dwHighDateTime;
        user.LowPart = user_time.dwLowDateTime;
        user.HighPart = user_time.dwHighDateTime;
        idle.LowPart = idle_time.dwLowDateTime;
        idle.HighPart = idle_time.dwHighDateTime;
        
        metrics.cpu_usage_percent = 100.0 - (idle.QuadPart * 100.0 / (kernel.QuadPart + user.QuadPart));
    }
    
    // Get memory usage
    PROCESS_MEMORY_COUNTERS_EX pmc;
    if (GetProcessMemoryInfo(GetCurrentProcess(), (PROCESS_MEMORY_COUNTERS*)&pmc, sizeof(pmc))) {
        metrics.memory_usage_mb = pmc.WorkingSetSize / (1024.0 * 1024.0);
    }
    
    // Update current resource metrics
    current_cpu_usage_ = metrics.cpu_usage_percent;
    current_memory_usage_ = metrics.memory_usage_mb;
}

void Benchmark::updateMetrics(const std::string& operation) {
    auto metrics = getMetrics(operation);
    
    // Store in history if real-time monitoring is enabled
    if (real_time_monitoring_) {
        metrics_history_.push_back(metrics);
        
        // Trim history if needed
        if (metrics_history_.size() > max_samples_) {
            metrics_history_.erase(
                metrics_history_.begin(),
                metrics_history_.begin() + (metrics_history_.size() - max_samples_)
            );
        }
    }
}

ResourceMetrics Benchmark::getCurrentResourceMetrics() const {
    ResourceMetrics metrics;
    metrics.cpu_usage_percent = current_cpu_usage_;
    metrics.memory_usage_mb = current_memory_usage_;
    return metrics;
}

void Benchmark::setSamplingInterval(std::chrono::milliseconds interval) {
    sampling_interval_ = interval;
}

void Benchmark::setMaxSamples(size_t max_samples) {
    max_samples_ = max_samples;
}

void Benchmark::enableRealTimeMonitoring(bool enable) {
    real_time_monitoring_ = enable;
}

void Benchmark::generateReport(const std::string& filename) {
    std::ofstream file(filename);
    if (!file.is_open()) {
        throw std::runtime_error("Failed to open file for writing: " + filename);
    }
    
    file << "Performance Report\n";
    file << "=================\n\n";
    
    auto metrics = getAllMetrics();
    for (const auto& metric : metrics) {
        file << "Operation: " << metric.operation_name << "\n";
        file << "  Latency (ms):\n";
        file << "    Min: " << metric.min_latency_ms << "\n";
        file << "    Max: " << metric.max_latency_ms << "\n";
        file << "    Avg: " << metric.average_latency_ms << "\n";
        file << "    P95: " << metric.p95_latency_ms << "\n";
        file << "    P99: " << metric.p99_latency_ms << "\n";
        file << "  Success Count: " << metric.success_count << "\n";
        file << "  Error Count: " << metric.error_count << "\n";
        file << "  CPU Usage: " << metric.cpu_usage << "%\n";
        file << "  Memory Usage: " << metric.memory_usage_mb << " MB\n\n";
    }
}

void Benchmark::plotMetrics(const std::string& output_dir) {
    // Create output directory if it doesn't exist
    std::filesystem::create_directories(output_dir);
    
    // Generate plots using a plotting library (placeholder)
    // In a real implementation, this would use a library like gnuplot or matplotlib
    // to create visualizations of the metrics
} 