#include "performance_dashboard.h"
#include <fstream>
#include <sstream>
#include <iomanip>
#include <filesystem>
#include <algorithm>
#include <nlohmann/json.hpp>

PerformanceDashboard::PerformanceDashboard()
    : start_time_(std::chrono::system_clock::now()) {
}

PerformanceDashboard::~PerformanceDashboard() {
    stop();
}

void PerformanceDashboard::initialize(const DashboardConfig& config) {
    std::lock_guard<std::mutex> lock(mutex_);
    config_ = config;
    std::filesystem::create_directories(config_.output_directory);
}

void PerformanceDashboard::start() {
    if (running_) return;
    
    running_ = true;
    update_thread_ = std::thread([this]() {
        while (running_) {
            update();
            std::this_thread::sleep_for(
                std::chrono::milliseconds(config_.update_interval_ms));
        }
    });
}

void PerformanceDashboard::stop() {
    if (!running_) return;
    
    running_ = false;
    if (update_thread_.joinable()) {
        update_thread_.join();
    }
}

void PerformanceDashboard::update() {
    std::lock_guard<std::mutex> lock(mutex_);
    
    // Get metrics from both benchmark and latency module
    auto benchmark_metrics = benchmark_.getAllMetrics();
    metrics_history_.insert(metrics_history_.end(), 
                          benchmark_metrics.begin(), 
                          benchmark_metrics.end());
    
    // Trim history if needed
    if (metrics_history_.size() > static_cast<size_t>(config_.max_history_points)) {
        metrics_history_.erase(
            metrics_history_.begin(),
            metrics_history_.begin() + 
                (metrics_history_.size() - config_.max_history_points));
    }
    
    if (config_.enable_html_reports) {
        generatePlots();
    }
    
    if (config_.enable_json_export || config_.enable_csv_export) {
        saveMetrics();
    }
    
    if (update_callback_) {
        update_callback_();
    }
}

void PerformanceDashboard::addCustomMetric(const std::string& name, double value) {
    std::lock_guard<std::mutex> lock(mutex_);
    custom_metrics_[name] = value;
}

void PerformanceDashboard::removeCustomMetric(const std::string& name) {
    std::lock_guard<std::mutex> lock(mutex_);
    custom_metrics_.erase(name);
}

std::string PerformanceDashboard::generateHTMLReport() const {
    std::stringstream ss;
    ss << generateHTMLHeader()
       << generateHTMLBody()
       << generateHTMLFooter();
    return ss.str();
}

void PerformanceDashboard::saveHTMLReport(const std::string& filename) const {
    std::ofstream file(filename);
    if (!file.is_open()) {
        throw std::runtime_error("Failed to open file for writing: " + filename);
    }
    file << generateHTMLReport();
}

void PerformanceDashboard::setUpdateCallback(std::function<void()> callback) {
    std::lock_guard<std::mutex> lock(mutex_);
    update_callback_ = callback;
}

void PerformanceDashboard::generatePlots() const {
    // Implementation using a plotting library (e.g., gnuplot or matplotlib)
    // This is a placeholder for the actual implementation
}

void PerformanceDashboard::saveMetrics() const {
    if (config_.enable_json_export) {
        saveMetricsJSON();
    }
    if (config_.enable_csv_export) {
        saveMetricsCSV();
    }
}

void PerformanceDashboard::saveMetricsJSON() const {
    nlohmann::json json_data;
    
    for (const auto& metric : metrics_history_) {
        nlohmann::json metric_json;
        metric_json["operation_name"] = metric.operation_name;
        metric_json["timestamp"] = std::chrono::system_clock::to_time_t(metric.timestamp);
        metric_json["average_latency_ms"] = metric.average_latency_ms;
        metric_json["min_latency_ms"] = metric.min_latency_ms;
        metric_json["max_latency_ms"] = metric.max_latency_ms;
        metric_json["p95_latency_ms"] = metric.p95_latency_ms;
        metric_json["p99_latency_ms"] = metric.p99_latency_ms;
        metric_json["success_count"] = metric.success_count;
        metric_json["error_count"] = metric.error_count;
        metric_json["cpu_usage"] = metric.cpu_usage;
        metric_json["memory_usage_mb"] = metric.memory_usage_mb;
        
        json_data.push_back(metric_json);
    }
    
    // Add custom metrics
    nlohmann::json custom_json;
    for (const auto& [name, value] : custom_metrics_) {
        custom_json[name] = value;
    }
    json_data["custom_metrics"] = custom_json;
    
    std::ofstream file(config_.output_directory + "/metrics.json");
    if (!file.is_open()) {
        throw std::runtime_error("Failed to open metrics.json for writing");
    }
    file << json_data.dump(4);
}

void PerformanceDashboard::saveMetricsCSV() const {
    std::ofstream file(config_.output_directory + "/metrics.csv");
    if (!file.is_open()) {
        throw std::runtime_error("Failed to open metrics.csv for writing");
    }
    
    // Write header
    file << "timestamp,operation_name,avg_latency_ms,min_latency_ms,max_latency_ms,"
         << "p95_latency_ms,p99_latency_ms,success_count,error_count,"
         << "cpu_usage,memory_usage_mb\n";
    
    // Write metrics
    for (const auto& metric : metrics_history_) {
        file << std::chrono::system_clock::to_time_t(metric.timestamp) << ","
             << metric.operation_name << ","
             << metric.average_latency_ms << ","
             << metric.min_latency_ms << ","
             << metric.max_latency_ms << ","
             << metric.p95_latency_ms << ","
             << metric.p99_latency_ms << ","
             << metric.success_count << ","
             << metric.error_count << ","
             << metric.cpu_usage << ","
             << metric.memory_usage_mb << "\n";
    }
    
    // Write custom metrics
    file << "\nCustom Metrics:\n";
    for (const auto& [name, value] : custom_metrics_) {
        file << name << "," << value << "\n";
    }
}

std::string PerformanceDashboard::generateHTMLHeader() const {
    return R"(
<!DOCTYPE html>
<html>
<head>
    <title>Performance Dashboard</title>
    <script src="https://cdn.plot.ly/plotly-latest.min.js"></script>
    <style>
        body { font-family: Arial, sans-serif; margin: 20px; }
        .plot-container { margin: 20px 0; }
        .metric-container { margin: 20px 0; }
        table { border-collapse: collapse; width: 100%; }
        th, td { border: 1px solid #ddd; padding: 8px; text-align: left; }
        th { background-color: #f2f2f2; }
    </style>
</head>
<body>
)";
}

std::string PerformanceDashboard::generateHTMLBody() const {
    std::stringstream ss;
    auto now = std::chrono::system_clock::now();
    auto time = std::chrono::system_clock::to_time_t(now);
    
    ss << "<h1>Performance Dashboard</h1>\n"
       << "<p>Last updated: " << std::put_time(std::localtime(&time), "%Y-%m-%d %H:%M:%S") << "</p>\n"
       << "<div class='metric-container'>\n"
       << "<h2>Performance Metrics</h2>\n"
       << generateMetricsTable()
       << "</div>\n";
    
    if (!custom_metrics_.empty()) {
        ss << "<div class='metric-container'>\n"
           << "<h2>Custom Metrics</h2>\n"
           << generateCustomMetricsTable()
           << "</div>\n";
    }
    
    return ss.str();
}

std::string PerformanceDashboard::generateHTMLFooter() const {
    return R"(
</body>
</html>
)";
}

std::string PerformanceDashboard::generateMetricsTable() const {
    // Implementation would generate HTML for displaying metrics
    return "<div id='metrics-table'></div>";
}

std::string PerformanceDashboard::generateCustomMetricsTable() const {
    std::stringstream ss;
    ss << "<table>\n"
       << "<tr><th>Metric</th><th>Value</th></tr>\n";
    
    for (const auto& [name, value] : custom_metrics_) {
        ss << "<tr><td>" << name << "</td><td>" 
           << std::fixed << std::setprecision(2) << value << "</td></tr>\n";
    }
    
    ss << "</table>\n";
    return ss.str();
} 