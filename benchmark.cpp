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

Benchmark::Benchmark()
    : sampling_interval_(std::chrono::milliseconds(100))
    , max_samples_(1000)
    , real_time_monitoring_(false)
    , resource_monitoring_active_(false)
    , stop_monitoring_(false) {
}

Benchmark::~Benchmark() {
    stopResourceMonitoring();
}

void Benchmark::startOperation(const std::string& operation_name) {
    std::lock_guard<std::mutex> lock(metrics_mutex_);
    auto& operation = operations_[operation_name];
    operation.start_time = std::chrono::system_clock::now();
}

void Benchmark::endOperation(const std::string& operation_name) {
    std::lock_guard<std::mutex> lock(metrics_mutex_);
    auto it = operations_.find(operation_name);
    if (it != operations_.end()) {
        auto end_time = std::chrono::system_clock::now();
        auto duration = std::chrono::duration_cast<std::chrono::microseconds>(
            end_time - it->second.start_time);
        it->second.latencies.push_back(duration);
        it->second.success_count++;
        
        if (real_time_monitoring_) {
            calculateLatencyMetrics(operation_name);
        }
    }
}

void Benchmark::recordError(const std::string& operation_name) {
    std::lock_guard<std::mutex> lock(metrics_mutex_);
    auto it = operations_.find(operation_name);
    if (it != operations_.end()) {
        it->second.failure_count++;
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
    GetSystemTimes(&idle_time, &kernel_time, &user_time);
    
    ULARGE_INTEGER kernel, user, idle;
    kernel.LowPart = kernel_time.dwLowDateTime;
    kernel.HighPart = kernel_time.dwHighDateTime;
    user.LowPart = user_time.dwLowDateTime;
    user.HighPart = user_time.dwHighDateTime;
    idle.LowPart = idle_time.dwLowDateTime;
    idle.HighPart = idle_time.dwHighDateTime;
    
    metrics.cpu_usage_percent = 100.0 - (idle.QuadPart * 100.0 / (kernel.QuadPart + user.QuadPart));
    
    // Get memory usage
    PROCESS_MEMORY_COUNTERS_EX pmc;
    if (GetProcessMemoryInfo(GetCurrentProcess(), (PROCESS_MEMORY_COUNTERS*)&pmc, sizeof(pmc))) {
        metrics.memory_usage_mb = pmc.WorkingSetSize / (1024.0 * 1024.0);
    }
    
    // Store metrics
    std::lock_guard<std::mutex> lock(metrics_mutex_);
    for (auto& op : operations_) {
        OperationMetrics op_metrics;
        op_metrics.operation_name = op.first;
        op_metrics.resources = metrics;
        op_metrics.timestamp = std::chrono::system_clock::now();
        metrics_history_.push_back(op_metrics);
    }
    
    // Trim history if needed
    if (metrics_history_.size() > max_samples_) {
        metrics_history_.erase(metrics_history_.begin(),
                             metrics_history_.begin() + (metrics_history_.size() - max_samples_));
    }
}

void Benchmark::calculateLatencyMetrics(const std::string& operation_name) {
    auto it = operations_.find(operation_name);
    if (it == operations_.end() || it->second.latencies.empty()) return;
    
    auto& latencies = it->second.latencies;
    std::sort(latencies.begin(), latencies.end());
    
    LatencyMetrics metrics;
    metrics.min_latency_ms = latencies.front().count() / 1000.0;
    metrics.max_latency_ms = latencies.back().count() / 1000.0;
    metrics.avg_latency_ms = std::accumulate(latencies.begin(), latencies.end(), 0.0,
        [](double sum, const auto& latency) { return sum + latency.count(); }) 
        / (latencies.size() * 1000.0);
    
    metrics.p50_latency_ms = latencies[latencies.size() * 0.5].count() / 1000.0;
    metrics.p90_latency_ms = latencies[latencies.size() * 0.9].count() / 1000.0;
    metrics.p99_latency_ms = latencies[latencies.size() * 0.99].count() / 1000.0;
    
    metrics.total_operations = latencies.size();
    metrics.successful_operations = it->second.success_count;
    metrics.failed_operations = it->second.failure_count;
    
    // Update metrics history
    std::lock_guard<std::mutex> lock(metrics_mutex_);
    for (auto& history : metrics_history_) {
        if (history.operation_name == operation_name) {
            history.latency = metrics;
        }
    }
}

void Benchmark::saveResults(const std::string& filename) {
    std::lock_guard<std::mutex> lock(metrics_mutex_);
    saveMetricsToFile(filename);
}

void Benchmark::loadResults(const std::string& filename) {
    std::lock_guard<std::mutex> lock(metrics_mutex_);
    loadMetricsFromFile(filename);
}

void Benchmark::saveMetricsToFile(const std::string& filename) {
    nlohmann::json json_data;
    
    for (const auto& metric : metrics_history_) {
        nlohmann::json metric_json;
        metric_json["operation_name"] = metric.operation_name;
        metric_json["timestamp"] = std::chrono::system_clock::to_time_t(metric.timestamp);
        
        metric_json["latency"]["min"] = metric.latency.min_latency_ms;
        metric_json["latency"]["max"] = metric.latency.max_latency_ms;
        metric_json["latency"]["avg"] = metric.latency.avg_latency_ms;
        metric_json["latency"]["p50"] = metric.latency.p50_latency_ms;
        metric_json["latency"]["p90"] = metric.latency.p90_latency_ms;
        metric_json["latency"]["p99"] = metric.latency.p99_latency_ms;
        
        metric_json["resources"]["cpu"] = metric.resources.cpu_usage_percent;
        metric_json["resources"]["memory"] = metric.resources.memory_usage_mb;
        metric_json["resources"]["network"] = metric.resources.network_bandwidth_mbps;
        
        json_data.push_back(metric_json);
    }
    
    std::ofstream file(filename);
    file << json_data.dump(4);
}

void Benchmark::loadMetricsFromFile(const std::string& filename) {
    std::ifstream file(filename);
    if (!file.is_open()) return;
    
    nlohmann::json json_data;
    file >> json_data;
    
    metrics_history_.clear();
    for (const auto& metric_json : json_data) {
        OperationMetrics metric;
        metric.operation_name = metric_json["operation_name"];
        metric.timestamp = std::chrono::system_clock::from_time_t(metric_json["timestamp"]);
        
        metric.latency.min_latency_ms = metric_json["latency"]["min"];
        metric.latency.max_latency_ms = metric_json["latency"]["max"];
        metric.latency.avg_latency_ms = metric_json["latency"]["avg"];
        metric.latency.p50_latency_ms = metric_json["latency"]["p50"];
        metric.latency.p90_latency_ms = metric_json["latency"]["p90"];
        metric.latency.p99_latency_ms = metric_json["latency"]["p99"];
        
        metric.resources.cpu_usage_percent = metric_json["resources"]["cpu"];
        metric.resources.memory_usage_mb = metric_json["resources"]["memory"];
        metric.resources.network_bandwidth_mbps = metric_json["resources"]["network"];
        
        metrics_history_.push_back(metric);
    }
}

void Benchmark::generateReport(const std::string& filename) {
    std::lock_guard<std::mutex> lock(metrics_mutex_);
    
    std::string extension = std::filesystem::path(filename).extension().string();
    if (extension == ".csv") {
        generateCSVReport(filename);
    } else if (extension == ".json") {
        generateJSONReport(filename);
    } else if (extension == ".html") {
        generateHTMLReport(filename);
    }
}

void Benchmark::generateCSVReport(const std::string& filename) {
    std::ofstream file(filename);
    file << "Operation,Timestamp,Min Latency,Max Latency,Avg Latency,P50,P90,P99,CPU,Memory,Network\n";
    
    for (const auto& metric : metrics_history_) {
        file << metric.operation_name << ","
             << std::chrono::system_clock::to_time_t(metric.timestamp) << ","
             << metric.latency.min_latency_ms << ","
             << metric.latency.max_latency_ms << ","
             << metric.latency.avg_latency_ms << ","
             << metric.latency.p50_latency_ms << ","
             << metric.latency.p90_latency_ms << ","
             << metric.latency.p99_latency_ms << ","
             << metric.resources.cpu_usage_percent << ","
             << metric.resources.memory_usage_mb << ","
             << metric.resources.network_bandwidth_mbps << "\n";
    }
}

void Benchmark::generateJSONReport(const std::string& filename) {
    saveMetricsToFile(filename);
}

void Benchmark::generateHTMLReport(const std::string& filename) {
    std::ofstream file(filename);
    file << "<!DOCTYPE html>\n<html>\n<head>\n"
         << "<title>Performance Report</title>\n"
         << "<script src=\"https://cdn.plot.ly/plotly-latest.min.js\"></script>\n"
         << "</head>\n<body>\n"
         << "<h1>Performance Report</h1>\n";
    
    // Generate plots for each operation
    for (const auto& op : operations_) {
        file << "<div id=\"plot_" << op.first << "\"></div>\n";
    }
    
    file << "<script>\n";
    
    // Generate JavaScript for each plot
    for (const auto& op : operations_) {
        file << "var data_" << op.first << " = {\n"
             << "  x: [";
        
        for (const auto& metric : metrics_history_) {
            if (metric.operation_name == op.first) {
                file << std::chrono::system_clock::to_time_t(metric.timestamp) << ",";
            }
        }
        
        file << "],\n  y: [";
        
        for (const auto& metric : metrics_history_) {
            if (metric.operation_name == op.first) {
                file << metric.latency.avg_latency_ms << ",";
            }
        }
        
        file << "],\n  type: 'scatter',\n"
             << "  name: '" << op.first << "'\n};\n"
             << "Plotly.newPlot('plot_" << op.first << "', [data_" << op.first << "]);\n";
    }
    
    file << "</script>\n</body>\n</html>";
}

void Benchmark::plotMetrics(const std::string& output_dir) {
    std::filesystem::create_directories(output_dir);
    plotLatencyGraphs(output_dir);
    plotResourceGraphs(output_dir);
}

void Benchmark::plotLatencyGraphs(const std::string& output_dir) {
    // Implementation would use a plotting library like gnuplot or matplotlib
    // This is a placeholder for the actual implementation
}

void Benchmark::plotResourceGraphs(const std::string& output_dir) {
    // Implementation would use a plotting library like gnuplot or matplotlib
    // This is a placeholder for the actual implementation
}

Benchmark::LatencyMetrics Benchmark::getLatencyMetrics(const std::string& operation_name) const {
    std::lock_guard<std::mutex> lock(metrics_mutex_);
    auto it = operations_.find(operation_name);
    if (it != operations_.end()) {
        LatencyMetrics metrics;
        calculateLatencyMetrics(operation_name);
        return metrics;
    }
    return LatencyMetrics();
}

Benchmark::ResourceMetrics Benchmark::getCurrentResourceMetrics() const {
    std::lock_guard<std::mutex> lock(metrics_mutex_);
    if (!metrics_history_.empty()) {
        return metrics_history_.back().resources;
    }
    return ResourceMetrics();
}

std::vector<Benchmark::OperationMetrics> Benchmark::getAllMetrics() const {
    std::lock_guard<std::mutex> lock(metrics_mutex_);
    return metrics_history_;
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