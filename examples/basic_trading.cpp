#include "../websocket_server.h"
#include "../error_handler.h"
#include "../performance_monitor.h"
#include <iostream>
#include <thread>
#include <chrono>

class BasicTradingExample {
public:
    BasicTradingExample() : ws_server_(9001) {
        // Initialize error handler
        ErrorHandler::getInstance().setErrorCallback(
            [this](const ErrorInfo& error) {
                handleError(error);
            }
        );
        
        // Initialize WebSocket handlers
        ws_server_.setMessageHandler(
            [this](const std::string& message) {
                handleMessage(message);
            }
        );
    }

    void run() {
        try {
            // Start performance monitoring
            auto& perfMon = PerformanceMonitor::getInstance();
            perfMon.startOperation("WebSocket Connection");

            // Start WebSocket server
            std::cout << "Starting WebSocket server on port 9001..." << std::endl;
            ws_server_.start();

            // Connect to Deribit test environment
            std::string connect_msg = R"({
                "jsonrpc": "2.0",
                "method": "public/auth",
                "params": {
                    "grant_type": "client_credentials",
                    "client_id": "YOUR_CLIENT_ID",
                    "client_secret": "YOUR_CLIENT_SECRET"
                }
            })";
            ws_server_.send(connect_msg);

            // Subscribe to BTC-PERPETUAL orderbook
            std::string subscribe_msg = R"({
                "jsonrpc": "2.0",
                "method": "public/subscribe",
                "params": {
                    "channels": ["book.BTC-PERPETUAL.100ms"]
                }
            })";
            ws_server_.send(subscribe_msg);

            // End performance monitoring for connection
            perfMon.endOperation("WebSocket Connection");

            // Keep the program running
            while (true) {
                std::this_thread::sleep_for(std::chrono::seconds(1));
            }
        }
        catch (const std::exception& e) {
            ErrorHandler::getInstance().logError(
                ErrorSeverity::CRITICAL,
                "Failed to run trading example",
                e.what()
            );
        }
    }

private:
    WebSocketServer ws_server_;

    void handleMessage(const std::string& message) {
        try {
            auto& perfMon = PerformanceMonitor::getInstance();
            perfMon.startOperation("Message Processing");

            // Parse and process the message
            std::cout << "Received message: " << message << std::endl;

            // Example of placing a limit order
            if (message.find("book.BTC-PERPETUAL.100ms") != std::string::npos) {
                std::string order_msg = R"({
                    "jsonrpc": "2.0",
                    "method": "private/buy",
                    "params": {
                        "instrument_name": "BTC-PERPETUAL",
                        "amount": 100,
                        "type": "limit",
                        "price": 50000
                    }
                })";
                ws_server_.send(order_msg);
            }

            perfMon.endOperation("Message Processing");
        }
        catch (const std::exception& e) {
            ErrorHandler::getInstance().logError(
                ErrorSeverity::ERROR,
                "Failed to process message",
                e.what()
            );
        }
    }

    void handleError(const ErrorInfo& error) {
        std::cout << "Error occurred: " << error.message
                  << " (Severity: " << static_cast<int>(error.severity) << ")"
                  << std::endl;

        if (error.severity == ErrorSeverity::CRITICAL) {
            std::cout << "Critical error occurred. Attempting recovery..." << std::endl;
            // Implement recovery logic here
        }
    }
};

int main() {
    BasicTradingExample example;
    example.run();
    return 0;
} 