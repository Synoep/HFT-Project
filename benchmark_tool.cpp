#include "benchmark.h"
#include "deribit_client.h"
#include <iostream>
#include <thread>
#include <chrono>
#include <iomanip>
#include <sstream>

class BenchmarkRunner {
public:
    BenchmarkRunner() : client_(DeribitClient::getInstance()) {
        // Initialize benchmark
        benchmark_.setSamplingInterval(std::chrono::milliseconds(100));
        benchmark_.setMaxSamples(1000);
        benchmark_.enableRealTimeMonitoring(true);
    }

    void runOrderPlacementBenchmark(int iterations = 100) {
        std::cout << "Running order placement benchmark..." << std::endl;
        
        for (int i = 0; i < iterations; ++i) {
            benchmark_.startOperation("place_order");
            
            DeribitClient::OrderRequest request;
            request.instrument = "BTC-PERPETUAL";
            request.side = "buy";
            request.size = 0.1;
            request.price = 50000.0;
            request.type = "limit";
            request.post_only = true;
            request.reduce_only = false;
            request.time_in_force = "good_til_cancelled";
            
            try {
                std::string order_id = client_.placeOrder(request);
                benchmark_.endOperation("place_order");
                
                // Cancel the order
                benchmark_.startOperation("cancel_order");
                client_.cancelOrder(order_id);
                benchmark_.endOperation("cancel_order");
            } catch (const std::exception& e) {
                benchmark_.recordError("place_order");
                std::cerr << "Error placing order: " << e.what() << std::endl;
            }
            
            std::this_thread::sleep_for(std::chrono::milliseconds(100));
        }
    }

    void runMarketDataBenchmark(int iterations = 100) {
        std::cout << "Running market data benchmark..." << std::endl;
        
        for (int i = 0; i < iterations; ++i) {
            benchmark_.startOperation("get_orderbook");
            try {
                auto orderbook = client_.getOrderBook("BTC-PERPETUAL");
                benchmark_.endOperation("get_orderbook");
            } catch (const std::exception& e) {
                benchmark_.recordError("get_orderbook");
                std::cerr << "Error getting orderbook: " << e.what() << std::endl;
            }
            
            std::this_thread::sleep_for(std::chrono::milliseconds(100));
        }
    }

    void runWebSocketBenchmark(int duration_seconds = 60) {
        std::cout << "Running WebSocket benchmark..." << std::endl;
        
        benchmark_.startResourceMonitoring();
        auto start_time = std::chrono::system_clock::now();
        
        while (std::chrono::duration_cast<std::chrono::seconds>(
               std::chrono::system_clock::now() - start_time).count() < duration_seconds) {
            benchmark_.startOperation("websocket_message");
            // Simulate WebSocket message processing
            std::this_thread::sleep_for(std::chrono::milliseconds(10));
            benchmark_.endOperation("websocket_message");
            
            std::this_thread::sleep_for(std::chrono::milliseconds(100));
        }
        
        benchmark_.stopResourceMonitoring();
    }

    void generateReport() {
        std::cout << "Generating benchmark report..." << std::endl;
        
        // Generate CSV report
        benchmark_.generateReport("benchmark_results.csv");
        
        // Generate HTML report with plots
        benchmark_.generateReport("benchmark_results.html");
        
        // Generate JSON report
        benchmark_.generateReport("benchmark_results.json");
        
        // Plot metrics
        benchmark_.plotMetrics("benchmark_plots");
        
        std::cout << "Reports generated in current directory." << std::endl;
    }

    void printSummary() {
        std::cout << "\nBenchmark Summary:\n";
        std::cout << "=================\n\n";
        
        auto metrics = benchmark_.getAllMetrics();
        for (const auto& metric : metrics) {
            std::cout << "Operation: " << metric.operation_name << "\n";
            std::cout << "  Latency (ms):\n";
            std::cout << "    Min: " << std::fixed << std::setprecision(2) 
                      << metric.latency.min_latency_ms << "\n";
            std::cout << "    Max: " << metric.latency.max_latency_ms << "\n";
            std::cout << "    Avg: " << metric.latency.avg_latency_ms << "\n";
            std::cout << "    P50: " << metric.latency.p50_latency_ms << "\n";
            std::cout << "    P90: " << metric.latency.p90_latency_ms << "\n";
            std::cout << "    P99: " << metric.latency.p99_latency_ms << "\n";
            std::cout << "  Success Rate: " 
                      << (metric.latency.successful_operations * 100.0 / 
                          (metric.latency.successful_operations + metric.latency.failed_operations))
                      << "%\n";
            std::cout << "  Resource Usage:\n";
            std::cout << "    CPU: " << metric.resources.cpu_usage_percent << "%\n";
            std::cout << "    Memory: " << metric.resources.memory_usage_mb << " MB\n";
            std::cout << "    Network: " << metric.resources.network_bandwidth_mbps << " Mbps\n\n";
        }
    }

private:
    Benchmark& benchmark_;
    DeribitClient& client_;
};

int main() {
    try {
        BenchmarkRunner runner;
        
        // Run benchmarks
        runner.runOrderPlacementBenchmark();
        runner.runMarketDataBenchmark();
        runner.runWebSocketBenchmark();
        
        // Generate reports
        runner.generateReport();
        runner.printSummary();
        
    } catch (const std::exception& e) {
        std::cerr << "Error running benchmarks: " << e.what() << std::endl;
        return 1;
    }
    
    return 0;
} 