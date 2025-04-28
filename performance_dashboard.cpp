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
            std::this_thread::sleep_for(config_.update_interval);
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
    
    // Update metrics from benchmark
    auto metrics = Benchmark::getInstance().getAllMetrics();
    metrics_history_.insert(metrics_history_.end(), metrics.begin(), metrics.end());
    
    // Trim history if needed
    if (metrics_history_.size() > config_.max_history_points) {
        metrics_history_.erase(metrics_history_.begin(),
                             metrics_history_.begin() + 
                             (metrics_history_.size() - config_.max_history_points));
    }
    
    // Generate new plots
    generatePlots();
    
    // Save metrics
    saveMetrics();
    
    // Notify callback if set
    if (update_callback_) {
        update_callback_();
    }
}

void PerformanceDashboard::addCustomMetric(const std::string& name,
                                         std::function<double()> getter) {
    std::lock_guard<std::mutex> lock(mutex_);
    custom_metrics_.emplace_back(name, getter);
}

void PerformanceDashboard::removeCustomMetric(const std::string& name) {
    std::lock_guard<std::mutex> lock(mutex_);
    custom_metrics_.erase(
        std::remove_if(custom_metrics_.begin(), custom_metrics_.end(),
                      [&name](const auto& pair) { return pair.first == name; }),
        custom_metrics_.end());
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
    file << generateHTMLReport();
}

void PerformanceDashboard::setUpdateCallback(std::function<void()> callback) {
    std::lock_guard<std::mutex> lock(mutex_);
    update_callback_ = callback;
}

void PerformanceDashboard::generatePlots() {
    // Implementation would use a plotting library like gnuplot or matplotlib
    // This is a placeholder for the actual implementation
}

void PerformanceDashboard::saveMetrics() {
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
    
    std::ofstream file(config_.output_directory + "/metrics.json");
    file << json_data.dump(4);
}

void PerformanceDashboard::loadMetrics() {
    std::ifstream file(config_.output_directory + "/metrics.json");
    if (!file.is_open()) return;
    
    nlohmann::json json_data;
    file >> json_data;
    
    metrics_history_.clear();
    for (const auto& metric_json : json_data) {
        Benchmark::OperationMetrics metric;
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
        table { border-collapse: collapse; width: 100%; }
        th, td { border: 1px solid #ddd; padding: 8px; text-align: left; }
        th { background-color: #f2f2f2; }
    </style>
</head>
)";
}

std::string PerformanceDashboard::generateHTMLBody() const {
    std::stringstream ss;
    ss << "<body>\n"
       << "<h1>Performance Dashboard</h1>\n"
       << "<p>Last updated: " 
       << std::chrono::system_clock::to_time_t(std::chrono::system_clock::now())
       << "</p>\n";
    
    if (config_.enable_latency_plot) {
        ss << "<div class='plot-container'>\n"
           << "<h2>Latency Metrics</h2>\n"
           << generateLatencyPlot()
           << "</div>\n";
    }
    
    if (config_.enable_resource_plot) {
        ss << "<div class='plot-container'>\n"
           << "<h2>Resource Usage</h2>\n"
           << generateResourcePlot()
           << "</div>\n";
    }
    
    if (config_.enable_error_plot) {
        ss << "<div class='plot-container'>\n"
           << "<h2>Error Rates</h2>\n"
           << generateErrorPlot()
           << "</div>\n";
    }
    
    ss << "<div class='metrics-container'>\n"
       << "<h2>Custom Metrics</h2>\n"
       << generateCustomMetricsTable()
       << "</div>\n";
    
    return ss.str();
}

std::string PerformanceDashboard::generateHTMLFooter() const {
    return R"(
</body>
</html>
)";
}

std::string PerformanceDashboard::generateLatencyPlot() const {
    // Implementation would generate Plotly.js code for latency plots
    return "<div id='latency-plot'></div>";
}

std::string PerformanceDashboard::generateResourcePlot() const {
    // Implementation would generate Plotly.js code for resource plots
    return "<div id='resource-plot'></div>";
}

std::string PerformanceDashboard::generateErrorPlot() const {
    // Implementation would generate Plotly.js code for error plots
    return "<div id='error-plot'></div>";
}

std::string PerformanceDashboard::generateCustomMetricsTable() const {
    std::stringstream ss;
    ss << "<table>\n"
       << "<tr><th>Metric</th><th>Value</th></tr>\n";
    
    for (const auto& metric : custom_metrics_) {
        try {
            double value = metric.second();
            ss << "<tr><td>" << metric.first << "</td><td>" 
               << std::fixed << std::setprecision(2) << value << "</td></tr>\n";
        } catch (...) {
            ss << "<tr><td>" << metric.first << "</td><td>Error</td></tr>\n";
        }
    }
    
    ss << "</table>\n";
    return ss.str();
} 