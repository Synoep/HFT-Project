#include "benchmark.h"
#include <gtest/gtest.h>
#include <thread>
#include <chrono>
#include <fstream>
#include <filesystem>

class BenchmarkTest : public ::testing::Test {
protected:
    void SetUp() override {
        benchmark_.setSamplingInterval(std::chrono::milliseconds(10));
        benchmark_.setMaxSamples(100);
        benchmark_.enableRealTimeMonitoring(false);
    }

    void TearDown() override {
        // Clean up test files
        std::filesystem::remove("test_benchmark.csv");
        std::filesystem::remove("test_benchmark.json");
        std::filesystem::remove("test_benchmark.html");
    }

    Benchmark& benchmark_;
};

TEST_F(BenchmarkTest, BasicOperation) {
    benchmark_.startOperation("test_operation");
    std::this_thread::sleep_for(std::chrono::milliseconds(50));
    benchmark_.endOperation("test_operation");

    auto metrics = benchmark_.getMetrics("test_operation");
    ASSERT_TRUE(metrics.has_value());
    EXPECT_GT(metrics->latency.avg_latency_ms, 0);
    EXPECT_EQ(metrics->latency.successful_operations, 1);
    EXPECT_EQ(metrics->latency.failed_operations, 0);
}

TEST_F(BenchmarkTest, ErrorRecording) {
    benchmark_.startOperation("test_operation");
    benchmark_.recordError("test_operation");
    benchmark_.endOperation("test_operation");

    auto metrics = benchmark_.getMetrics("test_operation");
    ASSERT_TRUE(metrics.has_value());
    EXPECT_EQ(metrics->latency.successful_operations, 0);
    EXPECT_EQ(metrics->latency.failed_operations, 1);
}

TEST_F(BenchmarkTest, MultipleOperations) {
    for (int i = 0; i < 10; ++i) {
        benchmark_.startOperation("test_operation");
        std::this_thread::sleep_for(std::chrono::milliseconds(10));
        benchmark_.endOperation("test_operation");
    }

    auto metrics = benchmark_.getMetrics("test_operation");
    ASSERT_TRUE(metrics.has_value());
    EXPECT_EQ(metrics->latency.successful_operations, 10);
    EXPECT_EQ(metrics->latency.failed_operations, 0);
}

TEST_F(BenchmarkTest, ReportGeneration) {
    benchmark_.startOperation("test_operation");
    std::this_thread::sleep_for(std::chrono::milliseconds(50));
    benchmark_.endOperation("test_operation");

    // Generate reports
    benchmark_.generateReport("test_benchmark.csv");
    benchmark_.generateReport("test_benchmark.json");
    benchmark_.generateReport("test_benchmark.html");

    // Verify files exist
    EXPECT_TRUE(std::filesystem::exists("test_benchmark.csv"));
    EXPECT_TRUE(std::filesystem::exists("test_benchmark.json"));
    EXPECT_TRUE(std::filesystem::exists("test_benchmark.html"));

    // Verify CSV content
    std::ifstream csv_file("test_benchmark.csv");
    std::string line;
    std::getline(csv_file, line);
    EXPECT_FALSE(line.empty());
    EXPECT_NE(line.find("test_operation"), std::string::npos);
}

TEST_F(BenchmarkTest, ResourceMonitoring) {
    benchmark_.enableRealTimeMonitoring(true);
    benchmark_.startResourceMonitoring();
    
    // Simulate some work
    for (int i = 0; i < 5; ++i) {
        benchmark_.startOperation("test_operation");
        std::this_thread::sleep_for(std::chrono::milliseconds(100));
        benchmark_.endOperation("test_operation");
    }
    
    benchmark_.stopResourceMonitoring();
    
    auto metrics = benchmark_.getMetrics("test_operation");
    ASSERT_TRUE(metrics.has_value());
    EXPECT_GT(metrics->resources.cpu_usage_percent, 0);
    EXPECT_GT(metrics->resources.memory_usage_mb, 0);
}

TEST_F(BenchmarkTest, LatencyPercentiles) {
    // Generate varying latencies
    for (int i = 0; i < 100; ++i) {
        benchmark_.startOperation("test_operation");
        std::this_thread::sleep_for(std::chrono::milliseconds(i));
        benchmark_.endOperation("test_operation");
    }

    auto metrics = benchmark_.getMetrics("test_operation");
    ASSERT_TRUE(metrics.has_value());
    
    // Verify percentiles
    EXPECT_GT(metrics->latency.p50_latency_ms, 0);
    EXPECT_GT(metrics->latency.p90_latency_ms, metrics->latency.p50_latency_ms);
    EXPECT_GT(metrics->latency.p99_latency_ms, metrics->latency.p90_latency_ms);
}

int main(int argc, char** argv) {
    ::testing::InitGoogleTest(&argc, argv);
    return RUN_ALL_TESTS();
} 