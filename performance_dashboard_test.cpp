#include "performance_dashboard.h"
#include <gtest/gtest.h>
#include <thread>
#include <chrono>
#include <fstream>
#include <filesystem>

class PerformanceDashboardTest : public ::testing::Test {
protected:
    void SetUp() override {
        // Initialize dashboard with test configuration
        PerformanceDashboard::DashboardConfig config;
        config.update_interval = std::chrono::milliseconds(10);
        config.max_history_points = 100;
        config.enable_latency_plot = true;
        config.enable_resource_plot = true;
        config.enable_error_plot = true;
        config.output_directory = "test_dashboard";
        
        dashboard_.initialize(config);
    }

    void TearDown() override {
        dashboard_.stop();
        // Clean up test files
        std::filesystem::remove_all("test_dashboard");
    }

    PerformanceDashboard& dashboard_;
};

TEST_F(PerformanceDashboardTest, Initialization) {
    EXPECT_TRUE(std::filesystem::exists("test_dashboard"));
}

TEST_F(PerformanceDashboardTest, CustomMetrics) {
    // Add custom metric
    bool metric_called = false;
    dashboard_.addCustomMetric("test_metric", [&metric_called]() {
        metric_called = true;
        return 42.0;
    });
    
    // Generate report
    std::string report = dashboard_.generateHTMLReport();
    
    // Check if metric was called and included in report
    EXPECT_TRUE(metric_called);
    EXPECT_NE(report.find("test_metric"), std::string::npos);
    EXPECT_NE(report.find("42.00"), std::string::npos);
}

TEST_F(PerformanceDashboardTest, ReportGeneration) {
    // Start dashboard
    dashboard_.start();
    
    // Wait for some updates
    std::this_thread::sleep_for(std::chrono::milliseconds(50));
    
    // Generate and save report
    dashboard_.saveHTMLReport("test_dashboard/report.html");
    
    // Check if report was created
    EXPECT_TRUE(std::filesystem::exists("test_dashboard/report.html"));
    
    // Check if metrics file was created
    EXPECT_TRUE(std::filesystem::exists("test_dashboard/metrics.json"));
}

TEST_F(PerformanceDashboardTest, UpdateCallback) {
    bool callback_called = false;
    dashboard_.setUpdateCallback([&callback_called]() {
        callback_called = true;
    });
    
    // Start dashboard
    dashboard_.start();
    
    // Wait for update
    std::this_thread::sleep_for(std::chrono::milliseconds(50));
    
    EXPECT_TRUE(callback_called);
}

TEST_F(PerformanceDashboardTest, MetricHistory) {
    // Add some test metrics
    for (int i = 0; i < 150; ++i) {
        Benchmark::OperationMetrics metric;
        metric.operation_name = "test_operation";
        metric.timestamp = std::chrono::system_clock::now();
        metric.latency.min_latency_ms = i;
        metric.latency.max_latency_ms = i + 1;
        metric.latency.avg_latency_ms = i + 0.5;
        
        dashboard_.update();
    }
    
    // Check if history is limited to max_points
    auto report = dashboard_.generateHTMLReport();
    size_t count = std::count(report.begin(), report.end(), "test_operation");
    EXPECT_LE(count, 100);  // Should not exceed max_history_points
}

TEST_F(PerformanceDashboardTest, ResourceMonitoring) {
    // Add resource monitoring metric
    dashboard_.addCustomMetric("cpu_usage", []() {
        return 75.5;  // Simulated CPU usage
    });
    
    dashboard_.addCustomMetric("memory_usage", []() {
        return 1024.0;  // Simulated memory usage in MB
    });
    
    // Generate report
    std::string report = dashboard_.generateHTMLReport();
    
    // Check if resource metrics are included
    EXPECT_NE(report.find("cpu_usage"), std::string::npos);
    EXPECT_NE(report.find("75.50"), std::string::npos);
    EXPECT_NE(report.find("memory_usage"), std::string::npos);
    EXPECT_NE(report.find("1024.00"), std::string::npos);
}

int main(int argc, char** argv) {
    ::testing::InitGoogleTest(&argc, argv);
    return RUN_ALL_TESTS();
} 