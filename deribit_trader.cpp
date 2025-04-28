#include "api_credentials.h"
#include "websocket_handler.h"
#include "websocket_server.h"
#include "trade_execution.h"
#include "latency_module.h"
#include <iostream>
#include <string>
#include <exception>
#include <memory>
#include <unordered_map>
#include <future>
#include <vector>
#include <thread>
#include <immintrin.h> // For SIMD Prepares for potential SIMD optimizations (not yet implemented in this code). 
                        //SIMD can be used for parallel processing of repetitive tasks like order book analysis.
                        //CPU Optimization
#include <chrono>
#include <atomic>

class TradingSystem {
public:
    TradingSystem() 
        : websocket_client_("test.deribit.com", "443", "/ws/api/v2"),
          websocket_server_("localhost", "8080"),
          trade_execution_(websocket_client_) {
    }

    void start() {
        try {
            // Initialize WebSocket client connection
            websocket_client_.connect();
            
            // Authenticate with Deribit
            auto auth_start = LatencyModule::start();
            json auth_response = trade_execution_.authenticate(CLIENT_ID, CLIENT_SECRET);
            auto auth_latency = std::chrono::duration_cast<LatencyModule::Duration>(
                std::chrono::high_resolution_clock::now() - auth_start);
            LatencyModule::trackWebSocketMessage(auth_latency);
            
            std::cout << "Auth Response: " << auth_response.dump(4) << std::endl;
            
            // Start WebSocket server in a separate thread
            std::thread server_thread([this]() {
                websocket_server_.start();
            });
            server_thread.detach();
            
            // Main trading loop
            while (running_) {
                display_menu();
                handle_user_input();
            }
            
            // Cleanup
            websocket_client_.close();
            websocket_server_.stop();
            
            // Save performance statistics
            LatencyModule::saveStats("performance_stats.csv");
            
        } catch (const std::exception& e) {
            std::cerr << "Error in TradingSystem: " << e.what() << std::endl;
        }
    }

private:
    void display_menu() {
        std::cout << "\n--- Trading Menu ---\n";
        std::cout << "1. Place Order\n";
        std::cout << "2. Cancel Order\n";
        std::cout << "3. Modify Order\n";
        std::cout << "4. Get Order Book\n";
        std::cout << "5. View Current Positions\n";
        std::cout << "6. Subscribe to Market Data\n";
        std::cout << "7. View Performance Stats\n";
        std::cout << "8. Exit\n";
        std::cout << "Enter your choice: ";
    }

    void handle_user_input() {
        int choice;
        std::cin >> choice;
        
        auto loop_start = LatencyModule::start();
        
        switch (choice) {
            case 1: handle_place_order(); break;
            case 2: handle_cancel_order(); break;
            case 3: handle_modify_order(); break;
            case 4: handle_get_orderbook(); break;
            case 5: handle_view_positions(); break;
            case 6: handle_subscribe_market_data(); break;
            case 7: handle_view_stats(); break;
            case 8: running_ = false; break;
            default: std::cout << "Invalid choice. Please try again.\n"; break;
        }
        
        auto loop_latency = std::chrono::duration_cast<LatencyModule::Duration>(
            std::chrono::high_resolution_clock::now() - loop_start);
        LatencyModule::trackTradingLoop(loop_latency);
    }

    void handle_place_order() {
        std::string instrument_name;
        double amount, price;
        
        std::cout << "Enter instrument name (e.g., BTC-PERPETUAL): ";
        std::cin >> instrument_name;
        std::cout << "Enter amount: ";
        std::cin >> amount;
        std::cout << "Enter price: ";
        std::cin >> price;
        
        try {
            auto order_start = LatencyModule::start();
            auto order_future = std::async(std::launch::async, [&]() {
                return trade_execution_.placeBuyOrder(instrument_name, amount, price);
            });
            
            json response = order_future.get();
            auto order_latency = std::chrono::duration_cast<LatencyModule::Duration>(
                std::chrono::high_resolution_clock::now() - order_start);
            LatencyModule::trackOrderPlacement(order_latency);
            
            std::cout << "Order Response: " << response.dump(4) << std::endl;
            
            // Broadcast order update to WebSocket clients
            websocket_server_.broadcast({
                {"type", "order_update"},
                {"data", response}
            });
        } catch (const std::exception& e) {
            std::cerr << "Error placing order: " << e.what() << std::endl;
        }
    }

    void handle_cancel_order() {
        std::string order_id;
        std::cout << "Enter order ID to cancel: ";
        std::cin >> order_id;
        
        try {
            auto cancel_start = LatencyModule::start();
            auto cancel_future = std::async(std::launch::async, [&]() {
                return trade_execution_.cancelOrder(order_id);
            });
            
            json response = cancel_future.get();
            auto cancel_latency = std::chrono::duration_cast<LatencyModule::Duration>(
                std::chrono::high_resolution_clock::now() - cancel_start);
            LatencyModule::trackOrderPlacement(cancel_latency);
            
            std::cout << "Cancel Response: " << response.dump(4) << std::endl;
            
            // Broadcast cancellation update
            websocket_server_.broadcast({
                {"type", "order_cancelled"},
                {"data", response}
            });
        } catch (const std::exception& e) {
            std::cerr << "Error cancelling order: " << e.what() << std::endl;
        }
    }

    void handle_modify_order() {
        std::string order_id;
        double new_price, new_amount;
        
        std::cout << "Enter order ID to modify: ";
        std::cin >> order_id;
        std::cout << "Enter new price: ";
        std::cin >> new_price;
        std::cout << "Enter new amount: ";
        std::cin >> new_amount;
        
        try {
            auto modify_start = LatencyModule::start();
            auto modify_future = std::async(std::launch::async, [&]() {
                return trade_execution_.modifyOrder(order_id, new_price, new_amount);
            });
            
            json response = modify_future.get();
            auto modify_latency = std::chrono::duration_cast<LatencyModule::Duration>(
                std::chrono::high_resolution_clock::now() - modify_start);
            LatencyModule::trackOrderPlacement(modify_latency);
            
            std::cout << "Modify Response: " << response.dump(4) << std::endl;
            
            // Broadcast modification update
            websocket_server_.broadcast({
                {"type", "order_modified"},
                {"data", response}
            });
        } catch (const std::exception& e) {
            std::cerr << "Error modifying order: " << e.what() << std::endl;
        }
    }

    void handle_get_orderbook() {
        std::string instrument_name;
        std::cout << "Enter instrument name to view order book: ";
        std::cin >> instrument_name;
        
        try {
            auto orderbook_start = LatencyModule::start();
            json orderbook = trade_execution_.getOrderBook(instrument_name);
            auto orderbook_latency = std::chrono::duration_cast<LatencyModule::Duration>(
                std::chrono::high_resolution_clock::now() - orderbook_start);
            LatencyModule::trackMarketData(orderbook_latency);
            
            std::cout << "Order Book: " << orderbook.dump(4) << std::endl;
            
            // Broadcast orderbook update
            websocket_server_.broadcast({
                {"type", "orderbook_update"},
                {"data", orderbook}
            });
        } catch (const std::exception& e) {
            std::cerr << "Error fetching order book: " << e.what() << std::endl;
        }
    }

    void handle_view_positions() {
        try {
            json positions = trade_execution_.getPositions();
            std::cout << "Current Positions: " << positions.dump(4) << std::endl;
            
            // Broadcast positions update
            websocket_server_.broadcast({
                {"type", "positions_update"},
                {"data", positions}
            });
        } catch (const std::exception& e) {
            std::cerr << "Error fetching positions: " << e.what() << std::endl;
        }
    }

    void handle_subscribe_market_data() {
        std::string instrument_name;
        std::cout << "Enter instrument name to subscribe: ";
        std::cin >> instrument_name;
        
        try {
            // Subscribe to market data updates
            json subscribe_msg = {
                {"jsonrpc", "2.0"},
                {"id", 1},
                {"method", "public/subscribe"},
                {"params", {
                    {"channels", {"book." + instrument_name + ".raw"}}
                }}
            };
            
            websocket_client_.sendMessage(subscribe_msg);
            std::cout << "Subscribed to market data for " << instrument_name << std::endl;
        } catch (const std::exception& e) {
            std::cerr << "Error subscribing to market data: " << e.what() << std::endl;
        }
    }

    void handle_view_stats() {
        auto order_stats = LatencyModule::getOrderPlacementStats();
        auto market_stats = LatencyModule::getMarketDataStats();
        auto ws_stats = LatencyModule::getWebSocketStats();
        auto loop_stats = LatencyModule::getTradingLoopStats();
        
        std::cout << "\n--- Performance Statistics ---\n";
        std::cout << "Order Placement:\n";
        print_stats(order_stats);
        std::cout << "\nMarket Data:\n";
        print_stats(market_stats);
        std::cout << "\nWebSocket Messages:\n";
        print_stats(ws_stats);
        std::cout << "\nTrading Loop:\n";
        print_stats(loop_stats);
    }

    void print_stats(const LatencyModule::LatencyStats& stats) {
        std::cout << "  Min: " << stats.min.count() << " ns\n";
        std::cout << "  Max: " << stats.max.count() << " ns\n";
        std::cout << "  Avg: " << stats.avg.count() << " ns\n";
        std::cout << "  P50: " << stats.p50.count() << " ns\n";
        std::cout << "  P90: " << stats.p90.count() << " ns\n";
        std::cout << "  P99: " << stats.p99.count() << " ns\n";
        std::cout << "  Count: " << stats.count << "\n";
    }

    WebSocketHandler websocket_client_;
    WebSocketServer websocket_server_;
    TradeExecution trade_execution_;
    std::atomic<bool> running_{true};
};

int main() {
    try {
        TradingSystem trading_system;
        trading_system.start();
    } catch (const std::exception& e) {
        std::cerr << "Error: " << e.what() << std::endl;
        return 1;
    }
    return 0;
}
