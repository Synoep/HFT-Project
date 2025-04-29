#include "benchmark.h"
#include <gtest/gtest.h>
#include <thread>
#include <chrono>
#include <fstream>
#include <filesystem>

class BenchmarkTest : public ::testing::Test {
protected:
    void SetUp() override {
        benchmark_ = &Benchmark::getInstance();
        benchmark_->setSamplingInterval(std::chrono::milliseconds(10));
        benchmark_->setMaxSamples(100);
        benchmark_->enableRealTimeMonitoring(false);
        benchmark_->reset();
    }

    void TearDown() override {
        benchmark_->reset();
        // Clean up test files
        std::filesystem::remove("test_benchmark.csv");
        std::filesystem::remove("test_benchmark.json");
        std::filesystem::remove("test_benchmark.html");
    }

    Benchmark* benchmark_;
};

TEST_F(BenchmarkTest, BasicOperation) {
    benchmark_->startOperation("test_operation");
    std::this_thread::sleep_for(std::chrono::milliseconds(50));
    benchmark_->endOperation("test_operation", true);

    auto metrics = benchmark_->getMetrics("test_operation");
    EXPECT_GT(metrics.average_latency_ms, 0);
    EXPECT_EQ(metrics.success_count, 1);
    EXPECT_EQ(metrics.error_count, 0);
}

TEST_F(BenchmarkTest, ErrorRecording) {
    benchmark_->startOperation("test_operation");
    benchmark_->recordError("test_operation", "Test error");
    benchmark_->endOperation("test_operation", false);

    auto metrics = benchmark_->getMetrics("test_operation");
    EXPECT_EQ(metrics.success_count, 0);
    EXPECT_EQ(metrics.error_count, 1);
}

TEST_F(BenchmarkTest, MultipleOperations) {
    for (int i = 0; i < 10; ++i) {
        benchmark_->startOperation("test_operation");
        std::this_thread::sleep_for(std::chrono::milliseconds(10));
        benchmark_->endOperation("test_operation", true);
    }

    auto metrics = benchmark_->getMetrics("test_operation");
    EXPECT_EQ(metrics.success_count, 10);
    EXPECT_EQ(metrics.error_count, 0);
}

TEST_F(BenchmarkTest, ReportGeneration) {
    benchmark_->startOperation("test_operation");
    std::this_thread::sleep_for(std::chrono::milliseconds(50));
    benchmark_->endOperation("test_operation", true);

    // Generate report
    benchmark_->generateReport("test_benchmark.csv");

    // Verify file exists
    EXPECT_TRUE(std::filesystem::exists("test_benchmark.csv"));

    // Verify CSV content
    std::ifstream csv_file("test_benchmark.csv");
    std::string line;
    std::getline(csv_file, line);
    EXPECT_FALSE(line.empty());
    EXPECT_NE(line.find("test_operation"), std::string::npos);
}

TEST_F(BenchmarkTest, ResourceMonitoring) {
    benchmark_->enableResourceMonitoring(true);
    
    // Simulate some work
    for (int i = 0; i < 5; ++i) {
        benchmark_->startOperation("test_operation");
        std::this_thread::sleep_for(std::chrono::milliseconds(100));
        benchmark_->endOperation("test_operation", true);
    }
    
    benchmark_->enableResourceMonitoring(false);
    
    auto metrics = benchmark_->getMetrics("test_operation");
    EXPECT_GT(metrics.cpu_usage, 0);
    EXPECT_GT(metrics.memory_usage_mb, 0);
}

TEST_F(BenchmarkTest, LatencyPercentiles) {
    // Generate varying latencies
    for (int i = 0; i < 100; ++i) {
        benchmark_->startOperation("test_operation");
        std::this_thread::sleep_for(std::chrono::milliseconds(i));
        benchmark_->endOperation("test_operation", true);
    }

    auto metrics = benchmark_->getMetrics("test_operation");
    
    // Verify percentiles are in order
    EXPECT_GT(metrics.p95_latency_ms, metrics.average_latency_ms);
    EXPECT_GT(metrics.p99_latency_ms, metrics.p95_latency_ms);
    EXPECT_GT(metrics.max_latency_ms, metrics.p99_latency_ms);
    EXPECT_LT(metrics.min_latency_ms, metrics.average_latency_ms);
}

TEST_F(BenchmarkTest, GetAllMetrics) {
    // Create multiple operations
    std::vector<std::string> operations = {"op1", "op2", "op3"};
    for (const auto& op : operations) {
        benchmark_->startOperation(op);
        std::this_thread::sleep_for(std::chrono::milliseconds(50));
        benchmark_->endOperation(op, true);
    }

    auto all_metrics = benchmark_->getAllMetrics();
    EXPECT_EQ(all_metrics.size(), operations.size());
    
    // Verify each operation is present
    std::set<std::string> found_ops;
    for (const auto& metric : all_metrics) {
        found_ops.insert(metric.operation_name);
        EXPECT_GT(metric.average_latency_ms, 0);
        EXPECT_EQ(metric.success_count, 1);
        EXPECT_EQ(metric.error_count, 0);
    }
    
    EXPECT_EQ(found_ops.size(), operations.size());
}

int main(int argc, char** argv) {
    ::testing::InitGoogleTest(&argc, argv);
    return RUN_ALL_TESTS();
} 