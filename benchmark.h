#ifndef BENCHMARK_H
#define BENCHMARK_H

#include <string>
#include <vector>
#include <map>
#include <mutex>
#include <thread>
#include <chrono>
#include <memory>
#include <atomic>
#include <fstream>
#include <nlohmann/json.hpp>
#include "latency_module.h"

class Benchmark {
public:
    struct OperationMetrics {
        std::string operation_name;
        double average_latency_ms{0.0};
        double min_latency_ms{0.0};
        double max_latency_ms{0.0};
        double p95_latency_ms{0.0};
        double p99_latency_ms{0.0};
        int success_count{0};
        int error_count{0};
        double cpu_usage{0.0};
        double memory_usage_mb{0.0};
        std::chrono::system_clock::time_point timestamp;
    };

    struct ResourceMetrics {
        double cpu_usage_percent{0.0};
        double memory_usage_mb{0.0};
        double network_bandwidth_mbps{0.0};
        size_t active_connections{0};
        size_t message_queue_size{0};
    };

    struct LatencyMetrics {
        double min_latency_ms{0.0};
        double max_latency_ms{0.0};
        double avg_latency_ms{0.0};
        double p50_latency_ms{0.0};
        double p90_latency_ms{0.0};
        double p99_latency_ms{0.0};
        size_t total_operations{0};
        size_t successful_operations{0};
        size_t failed_operations{0};
    };

    static Benchmark& getInstance() {
        static Benchmark instance;
        return instance;
    }

    void startOperation(const std::string& name);
    void endOperation(const std::string& name, bool success = true);
    void recordError(const std::string& operation, const std::string& error_message);
    OperationMetrics getMetrics(const std::string& operation) const;
    std::vector<OperationMetrics> getAllMetrics() const;
    void reset();
    void enableResourceMonitoring(bool enable);
    
    void startResourceMonitoring();
    void stopResourceMonitoring();
    
    void saveResults(const std::string& filename);
    void loadResults(const std::string& filename);
    
    LatencyMetrics getLatencyMetrics(const std::string& operation_name) const;
    ResourceMetrics getCurrentResourceMetrics() const;
    void setSamplingInterval(std::chrono::milliseconds interval);
    void setMaxSamples(size_t max_samples);
    void enableRealTimeMonitoring(bool enable);
    
    void generateReport(const std::string& filename);
    void plotMetrics(const std::string& output_dir);

private:
    Benchmark();
    ~Benchmark();
    Benchmark(const Benchmark&) = delete;
    Benchmark& operator=(const Benchmark&) = delete;

    void monitorResources();
    void updateMetrics(const std::string& operation);
    void updateResourceMetrics();
    void calculateLatencyMetrics(const std::string& operation_name);
    void saveMetricsToFile(const std::string& filename);
    void loadMetricsFromFile(const std::string& filename);
    void generateCSVReport(const std::string& filename);
    void generateJSONReport(const std::string& filename);
    void generateHTMLReport(const std::string& filename);
    void plotLatencyGraphs(const std::string& output_dir);
    void plotResourceGraphs(const std::string& output_dir);

    struct OperationData {
        std::vector<double> latencies;
        int success_count{0};
        int error_count{0};
        std::chrono::steady_clock::time_point start_time;
    };

    mutable std::mutex metrics_mutex_;
    std::map<std::string, OperationData> operations_;
    std::thread resource_monitoring_thread_;
    std::atomic<bool> monitoring_enabled_{false};
    std::atomic<double> current_cpu_usage_{0.0};
    std::atomic<double> current_memory_usage_{0.0};
    std::chrono::milliseconds sampling_interval_{100};
    size_t max_samples_{1000};
    bool real_time_monitoring_{false};
    bool resource_monitoring_active_{false};
    std::atomic<bool> stop_monitoring_{false};
    
    // Reference to the latency module for detailed latency tracking
    LatencyModule& latency_module_{LatencyModule::getInstance()};
};

#endif // BENCHMARK_H 