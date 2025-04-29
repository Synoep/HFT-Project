#ifndef PERFORMANCE_DASHBOARD_H
#define PERFORMANCE_DASHBOARD_H

#include <string>
#include <vector>
#include <map>
#include <mutex>
#include <thread>
#include <chrono>
#include <functional>
#include <memory>
#include "benchmark.h"
#include "latency_module.h"

class PerformanceDashboard {
public:
    struct DashboardConfig {
        std::string output_directory{"./dashboard"};
        int update_interval_ms{1000};
        int max_history_points{1000};
        bool enable_html_reports{true};
        bool enable_json_export{true};
        bool enable_csv_export{true};
    };

    static PerformanceDashboard& getInstance() {
        static PerformanceDashboard instance;
        return instance;
    }

    void initialize(const DashboardConfig& config);
    void start();
    void stop();
    void addCustomMetric(const std::string& name, double value);
    void removeCustomMetric(const std::string& name);
    void setUpdateCallback(std::function<void()> callback);
    std::string generateHTMLReport() const;
    void saveHTMLReport(const std::string& filename) const;
    void generatePlots() const;
    void saveMetrics() const;

private:
    PerformanceDashboard() = default;
    ~PerformanceDashboard();
    PerformanceDashboard(const PerformanceDashboard&) = delete;
    PerformanceDashboard& operator=(const PerformanceDashboard&) = delete;

    void update();
    std::string generateHTMLHeader() const;
    std::string generateHTMLBody() const;
    std::string generateHTMLFooter() const;

    DashboardConfig config_;
    std::vector<Benchmark::OperationMetrics> metrics_history_;
    std::map<std::string, double> custom_metrics_;
    mutable std::mutex mutex_;
    std::thread update_thread_;
    std::function<void()> update_callback_;
    bool running_{false};
    
    // References to other modules
    Benchmark& benchmark_{Benchmark::getInstance()};
    LatencyModule& latency_module_{LatencyModule::getInstance()};
};

#endif // PERFORMANCE_DASHBOARD_H 