#include "latency_module.h"
#include <gtest/gtest.h>
#include <thread>
#include <filesystem>
#include <fstream>

class LatencyModuleTest : public ::testing::Test {
protected:
    void SetUp() override {
        module = std::make_unique<LatencyModule>();
    }

    void TearDown() override {
        // Clean up any log files
        for (const auto& entry : std::filesystem::directory_iterator(".")) {
            if (entry.path().string().find("latency_log_") != std::string::npos) {
                std::filesystem::remove(entry.path());
            }
        }
        if (std::filesystem::exists("test_stats.csv")) {
            std::filesystem::remove("test_stats.csv");
        }
    }

    std::unique_ptr<LatencyModule> module;

    void simulateLatency(const std::string& operation_id) {
        auto start_time = module->start(operation_id);
        std::this_thread::sleep_for(std::chrono::milliseconds(1));
        module->end(operation_id, start_time);
    }
};

TEST_F(LatencyModuleTest, BasicMeasurement) {
    auto start_time = module->start("test_op");
    std::this_thread::sleep_for(std::chrono::milliseconds(1));
    module->end("test_op", start_time);
}

TEST_F(LatencyModuleTest, OrderPlacementTracking) {
    for (int i = 0; i < 10; i++) {
        auto start_time = module->start("order_placement");
        std::this_thread::sleep_for(std::chrono::milliseconds(1));
        module->end("order_placement", start_time);
    }
    
    auto stats = module->getOrderPlacementStats();
    EXPECT_GT(stats.count, 0);
    EXPECT_GT(stats.avg, 0);
    EXPECT_GE(stats.max, stats.avg);
    EXPECT_LE(stats.min, stats.avg);
}

TEST_F(LatencyModuleTest, MarketDataTracking) {
    for (int i = 0; i < 10; i++) {
        auto start_time = module->start("market_data");
        std::this_thread::sleep_for(std::chrono::microseconds(100));
        module->end("market_data", start_time);
    }
    
    auto stats = module->getMarketDataStats();
    EXPECT_GT(stats.count, 0);
    EXPECT_GT(stats.avg, 0);
    EXPECT_GE(stats.p99, stats.p90);
    EXPECT_GE(stats.p90, stats.p50);
}

TEST_F(LatencyModuleTest, WebSocketTracking) {
    for (int i = 0; i < 10; i++) {
        auto start_time = module->start("websocket");
        std::this_thread::sleep_for(std::chrono::microseconds(50));
        module->end("websocket", start_time);
    }
    
    auto stats = module->getWebSocketStats();
    EXPECT_GT(stats.count, 0);
    EXPECT_GT(stats.avg, 0);
}

TEST_F(LatencyModuleTest, TradingLoopTracking) {
    for (int i = 0; i < 10; i++) {
        auto start_time = module->start("trading_loop");
        std::this_thread::sleep_for(std::chrono::milliseconds(2));
        module->end("trading_loop", start_time);
    }
    
    auto stats = module->getTradingLoopStats();
    EXPECT_GT(stats.count, 0);
    EXPECT_GT(stats.avg, 0);
}

TEST_F(LatencyModuleTest, StatsSaving) {
    // Generate some data
    for (int i = 0; i < 5; i++) {
        simulateLatency("order_placement");
        simulateLatency("market_data");
        simulateLatency("websocket");
        simulateLatency("trading_loop");
    }
    
    module->saveStats("test_stats.csv");
    EXPECT_TRUE(std::filesystem::exists("test_stats.csv"));
    
    // Verify file content
    std::ifstream file("test_stats.csv");
    EXPECT_TRUE(file.is_open());
    
    std::string header;
    std::getline(file, header);
    EXPECT_FALSE(header.empty());
    EXPECT_NE(header.find("Operation"), std::string::npos);
}

TEST_F(LatencyModuleTest, StatsReset) {
    // Generate some data
    for (int i = 0; i < 5; i++) {
        simulateLatency("order_placement");
    }
    
    auto stats_before = module->getOrderPlacementStats();
    EXPECT_GT(stats_before.count, 0);
    
    module->resetStats();
    
    auto stats_after = module->getOrderPlacementStats();
    EXPECT_EQ(stats_after.count, 0);
}

TEST_F(LatencyModuleTest, LogFileCreation) {
    simulateLatency("test_operation");
    
    bool found_log_file = false;
    for (const auto& entry : std::filesystem::directory_iterator(".")) {
        if (entry.path().string().find("latency_log_") != std::string::npos) {
            found_log_file = true;
            break;
        }
    }
    EXPECT_TRUE(found_log_file);
}

TEST_F(LatencyModuleTest, HistorySizeLimit) {
    const int max_size = 1000;  // Default max history size
    
    // Generate more than max_size entries
    for (int i = 0; i < max_size + 100; i++) {
        simulateLatency("order_placement");
    }
    
    auto stats = module->getOrderPlacementStats();
    EXPECT_LE(stats.count, max_size);
}

int main(int argc, char **argv) {
    testing::InitGoogleTest(&argc, argv);
    return RUN_ALL_TEST();
} 