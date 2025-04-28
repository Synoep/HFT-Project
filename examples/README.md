# WebSocket HFT Examples

This directory contains example programs demonstrating the usage of the WebSocket HFT system.

## Basic Trading Example

The `basic_trading.cpp` example demonstrates the core functionality of the WebSocket HFT system, including:

- WebSocket server setup and connection
- Error handling and logging
- Performance monitoring
- Basic order management
- Market data subscription

### Features Demonstrated

1. **Error Handling**
   - Comprehensive error logging
   - Error severity levels (INFO, WARNING, ERROR, CRITICAL)
   - Error callbacks and recovery mechanisms

2. **Performance Monitoring**
   - Operation timing
   - Resource usage tracking
   - Performance statistics collection

3. **WebSocket Communication**
   - Connection to Deribit test environment
   - Market data subscription
   - Order placement

### Building and Running

1. Ensure you have built the main project:
   ```bash
   cd build
   cmake --build . --config Release
   ```

2. The example executable will be available at:
   ```
   build/bin/Release/basic_trading_example.exe
   ```

3. Before running, update the following in `basic_trading.cpp`:
   - Replace `YOUR_CLIENT_ID` with your Deribit API client ID
   - Replace `YOUR_CLIENT_SECRET` with your Deribit API client secret

4. Run the example:
   ```bash
   build/bin/Release/basic_trading_example.exe
   ```

### Expected Output

The program will:
1. Start a WebSocket server on port 9001
2. Connect to the Deribit test environment
3. Subscribe to BTC-PERPETUAL orderbook updates
4. Process incoming messages and place example orders
5. Log performance metrics and any errors that occur

### Performance Monitoring

The example includes performance monitoring for:
- WebSocket connection setup
- Message processing
- Order placement

Performance statistics are automatically collected and can be accessed through the `PerformanceMonitor` class.

### Error Handling

The example demonstrates error handling for:
- Connection failures
- Message processing errors
- Order placement errors

Errors are logged with appropriate severity levels and trigger recovery mechanisms when necessary.

### Next Steps

After understanding the basic example, you can:
1. Modify the order parameters and trading logic
2. Add more sophisticated error recovery mechanisms
3. Implement additional performance monitoring metrics
4. Add more complex trading strategies 