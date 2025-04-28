#ifndef BENCHMARK_H
#define BENCHMARK_H

#include <string>
#include <chrono>
#include <vector>
#include <map>
#include <memory>
#include <fstream>
#include <nlohmann/json.hpp>

class Benchmark {
public:
    struct LatencyMetrics {
        double min_latency_ms;
        double max_latency_ms;
        double avg_latency_ms;
        double p50_latency_ms;
        double p90_latency_ms;
        double p99_latency_ms;
        size_t total_operations;
        size_t successful_operations;
        size_t failed_operations;
    };

    struct ResourceMetrics {
        double cpu_usage_percent;
        double memory_usage_mb;
        double network_bandwidth_mbps;
        size_t active_connections;
        size_t message_queue_size;
    };

    struct OperationMetrics {
        std::string operation_name;
        LatencyMetrics latency;
        ResourceMetrics resources;
        std::chrono::system_clock::time_point timestamp;
    };

    static Benchmark& getInstance() {
        static Benchmark instance;
        return instance;
    }

    void startOperation(const std::string& operation_name);
    void endOperation(const std::string& operation_name);
    void recordError(const std::string& operation_name);
    
    void startResourceMonitoring();
    void stopResourceMonitoring();
    
    void saveResults(const std::string& filename);
    void loadResults(const std::string& filename);
    
    LatencyMetrics getLatencyMetrics(const std::string& operation_name) const;
    ResourceMetrics getCurrentResourceMetrics() const;
    std::vector<OperationMetrics> getAllMetrics() const;
    
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
        std::vector<std::chrono::microseconds> latencies;
        size_t success_count;
        size_t failure_count;
        std::chrono::system_clock::time_point start_time;
    };

    std::map<std::string, OperationData> operations_;
    std::vector<OperationMetrics> metrics_history_;
    std::chrono::milliseconds sampling_interval_;
    size_t max_samples_;
    bool real_time_monitoring_;
    bool resource_monitoring_active_;
    std::thread resource_monitoring_thread_;
    std::mutex metrics_mutex_;
    std::atomic<bool> stop_monitoring_;
};

#endif // BENCHMARK_H 