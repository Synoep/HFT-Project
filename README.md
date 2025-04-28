# WebSocket HFT Trading System

A high-frequency trading system built with C++ and WebSocket technology for the Deribit exchange.

## Features

### Core Trading System
- WebSocket-based real-time market data
- Order management (place, cancel, modify)
- Position management
- Market data streaming
- Multi-threaded architecture

### Error Handling
- Comprehensive error logging
- Automatic error recovery
- Error notification system
- Stack trace capture
- Log rotation and management

### Performance Monitoring
- Real-time latency tracking
- Resource usage monitoring
- Performance metrics collection
- Custom metric support
- Interactive dashboard

### Benchmarking
- Operation timing
- Success/failure tracking
- Resource usage tracking
- Report generation (CSV, JSON, HTML)
- Latency percentile calculation

## Building

### Prerequisites
- CMake 3.15 or higher
- C++17 compatible compiler
- Boost libraries
- OpenSSL
- nlohmann-json

### Build Steps
```bash
mkdir build
cd build
cmake .. -G "Visual Studio 17 2022" -A x64 -DCMAKE_TOOLCHAIN_FILE=C:/vcpkg/scripts/buildsystems/vcpkg.cmake
cmake --build . --config Release
```

## Usage

### Basic Trading
```cpp
#include "deribit_client.h"

int main() {
    auto& client = DeribitClient::getInstance();
    client.initialize("your_api_key", "your_api_secret");
    
    // Place an order
    DeribitClient::OrderRequest request;
    request.instrument = "BTC-PERPETUAL";
    request.side = "buy";
    request.size = 0.1;
    request.price = 50000.0;
    request.type = "limit";
    
    std::string order_id = client.placeOrder(request);
    
    // Cancel the order
    client.cancelOrder(order_id);
    
    return 0;
}
```

### Error Handling
```cpp
#include "error_handler.h"

// Log errors
LOG_INFO("Operation started", "context");
LOG_WARNING("Potential issue", "context");
LOG_ERROR("Operation failed", "context");
LOG_CRITICAL("System error", "context");

// Add recovery actions
ErrorHandler::RecoveryAction action;
action.name = "network_recovery";
action.action = []() { return reconnectNetwork(); };
action.priority = 1;
action.max_attempts = 3;
action.retry_interval = std::chrono::seconds(1);

ErrorHandler::getInstance().addRecoveryAction(action);
```

### Performance Monitoring
```cpp
#include "performance_dashboard.h"

// Initialize dashboard
PerformanceDashboard::DashboardConfig config;
config.update_interval = std::chrono::seconds(1);
config.max_history_points = 1000;
config.enable_latency_plot = true;
config.enable_resource_plot = true;
config.enable_error_plot = true;

auto& dashboard = PerformanceDashboard::getInstance();
dashboard.initialize(config);
dashboard.start();

// Add custom metrics
dashboard.addCustomMetric("order_queue_size", []() {
    return getOrderQueueSize();
});

// Generate report
dashboard.saveHTMLReport("performance_report.html");
```

### Benchmarking
```cpp
#include "benchmark.h"

// Start operation timing
Benchmark::getInstance().startOperation("order_placement");

try {
    // Your code here
    // ...
    
    // End operation (success)
    Benchmark::getInstance().endOperation("order_placement");
} catch (const std::exception& e) {
    // End operation (failure)
    Benchmark::getInstance().endOperation("order_placement", e.what());
}

// Generate reports
Benchmark::getInstance().generateReport("benchmark_results.csv");
Benchmark::getInstance().generateReport("benchmark_results.json");
Benchmark::getInstance().generateReport("benchmark_results.html");
```

## Configuration

### Error Handler Configuration
- Log directory: `logs/`
- Max log size: 10MB
- Log rotation count: 5
- Recovery enabled: true

### Performance Dashboard Configuration
- Update interval: 1 second
- Max history points: 1000
- Output directory: `dashboard/`
- Enabled plots: latency, resources, errors

### Benchmark Configuration
- Sampling interval: 100ms
- Max samples: 1000
- Real-time monitoring: enabled

## Performance Optimization

### Memory Management
- Use memory pools for frequent allocations
- Implement object reuse patterns
- Minimize dynamic memory allocation
- Use stack allocation where possible

### Network Optimization
- Use connection pooling
- Implement message batching
- Optimize WebSocket frame size
- Use compression where beneficial

### CPU Optimization
- Use thread pools
- Implement lock-free algorithms
- Optimize hot paths
- Use SIMD instructions where applicable

## Error Recovery

### Network Errors
- Automatic reconnection
- Connection state tracking
- Message queue management
- Session recovery

### System Errors
- Memory leak detection
- Resource cleanup
- State restoration
- Graceful degradation

## Monitoring and Metrics

### Latency Metrics
- Min/Max/Average latency
- P50/P90/P99 percentiles
- Operation success rate
- Error distribution

### Resource Metrics
- CPU usage
- Memory usage
- Network bandwidth
- Active connections

### Custom Metrics
- Order queue size
- Position delta
- PnL tracking
- Risk metrics

## Contributing

1. Fork the repository
2. Create a feature branch
3. Commit your changes
4. Push to the branch
5. Create a Pull Request

## License

This project is licensed under the MIT License - see the LICENSE file for details.

## Disclaimer

This software is for educational purposes only. Trading cryptocurrency derivatives carries significant risk. Use at your own risk.

## Acknowledgments

- Deribit for providing the test environment
- Boost C++ Libraries
- OpenSSL
- nlohmann/json










