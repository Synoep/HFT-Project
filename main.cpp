#include "benchmark.h"
#include "latency_module.h"
#include "performance_dashboard.h"
#include <iostream>
#include <thread>
#include <chrono>
#include <random>

// Simulated trading operation
void simulateTrading(const std::string& operation_name, int duration_ms) {
    auto& benchmark = Benchmark::getInstance();
    benchmark.startOperation(operation_name);
    
    // Simulate some work
    std::this_thread::sleep_for(std::chrono::milliseconds(duration_ms));
    
    benchmark.endOperation(operation_name, true);
}

// Simulated market data processing
void simulateMarketData(const std::string& symbol, int count) {
    auto& benchmark = Benchmark::getInstance();
    std::random_device rd;
    std::mt19937 gen(rd());
    std::uniform_int_distribution<> dis(1, 20);

    for (int i = 0; i < count; ++i) {
        std::string operation = "market_data_" + symbol;
        benchmark.startOperation(operation);
        
        // Simulate variable processing time
        std::this_thread::sleep_for(std::chrono::milliseconds(dis(gen)));
        
        benchmark.endOperation(operation, true);
    }
}

int main() {
    try {
        // Initialize components
        auto& benchmark = Benchmark::getInstance();
        auto& dashboard = PerformanceDashboard::getInstance();

        // Configure dashboard
        PerformanceDashboard::DashboardConfig config;
        config.output_directory = "performance_data";
        config.update_interval_ms = 1000;
        config.max_history_points = 1000;
        config.enable_html_reports = true;
        config.enable_json_export = true;
        config.enable_csv_export = true;

        // Initialize and start dashboard
        dashboard.initialize(config);
        dashboard.start();

        // Enable resource monitoring
        benchmark.enableResourceMonitoring(true);
        benchmark.setMaxSamples(1000);
        benchmark.setSamplingInterval(std::chrono::milliseconds(100));
        benchmark.enableRealTimeMonitoring(true);

        std::cout << "Starting performance monitoring demo...\n";

        // Simulate different operations
        for (int i = 0; i < 10; ++i) {
            // Simulate order placement
            simulateTrading("order_placement", 50);

            // Simulate market data processing for different symbols
            std::thread t1(simulateMarketData, "BTC-USD", 5);
            std::thread t2(simulateMarketData, "ETH-USD", 5);

            // Simulate order cancellation
            simulateTrading("order_cancellation", 30);

            // Wait for market data threads to complete
            t1.join();
            t2.join();

            // Add some custom metrics
            dashboard.addCustomMetric("iteration", static_cast<double>(i));
            dashboard.addCustomMetric("active_orders", static_cast<double>(i * 2));

            std::cout << "Completed iteration " << (i + 1) << " of 10\n";
            std::this_thread::sleep_for(std::chrono::milliseconds(500));
        }

        // Generate final reports
        std::cout << "\nGenerating performance reports...\n";
        
        // Save HTML report
        dashboard.saveHTMLReport("performance_data/dashboard.html");
        
        // Generate benchmark report
        benchmark.generateReport("performance_data/benchmark_report.txt");
        
        // Plot metrics
        benchmark.plotMetrics("performance_data/plots");

        // Stop monitoring
        benchmark.enableResourceMonitoring(false);
        dashboard.stop();

        std::cout << "\nPerformance monitoring demo completed.\n";
        std::cout << "Reports have been generated in the 'performance_data' directory.\n";
        
    } catch (const std::exception& e) {
        std::cerr << "Error: " << e.what() << std::endl;
        return 1;
    }

    return 0;
} 