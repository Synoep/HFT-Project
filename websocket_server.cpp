#include "websocket_server.h"
#include <iostream>
#include <sstream>
#include <chrono>
#include <functional>
#include <filesystem>

WebSocketServer::WebSocketServer(const std::string& host, const std::string& port)
    : host_(host), port_(port), acceptor_(ioc_), running_(false) {
    
    // Create logs directory if it doesn't exist
    std::filesystem::create_directory("logs");
    
    // Open log files
    error_log_.open("logs/error.log", std::ios::app);
    info_log_.open("logs/info.log", std::ios::app);
    
    start_time_ = std::chrono::steady_clock::now();
    
    try {
        tcp::endpoint endpoint(tcp::v4(), std::stoi(port));
        acceptor_.open(endpoint.protocol());
        acceptor_.set_option(asio::socket_base::reuse_address(true));
        acceptor_.bind(endpoint);
        acceptor_.listen();
        log_info("Server initialized successfully", "constructor");
    } catch (const std::exception& e) {
        log_error("Failed to initialize server: " + std::string(e.what()), "constructor");
        throw;
    }
}

WebSocketServer::~WebSocketServer() {
    stop();
    error_log_.close();
    info_log_.close();
}

void WebSocketServer::log_error(const std::string& error_message, const std::string& context) {
    auto now = std::chrono::system_clock::now();
    auto time = std::chrono::system_clock::to_time_t(now);
    std::stringstream ss;
    ss << std::put_time(std::localtime(&time), "%Y-%m-%d %H:%M:%S");
    
    std::string log_entry = "[" + ss.str() + "] [" + context + "] ERROR: " + error_message + "\n";
    error_log_ << log_entry;
    error_log_.flush();
    std::cerr << log_entry;
}

void WebSocketServer::log_info(const std::string& info_message, const std::string& context) {
    auto now = std::chrono::system_clock::now();
    auto time = std::chrono::system_clock::to_time_t(now);
    std::stringstream ss;
    ss << std::put_time(std::localtime(&time), "%Y-%m-%d %H:%M:%S");
    
    std::string log_entry = "[" + ss.str() + "] [" + context + "] INFO: " + info_message + "\n";
    info_log_ << log_entry;
    info_log_.flush();
    std::cout << log_entry;
}

void WebSocketServer::handle_connection_error(beast::error_code ec, const std::string& context) {
    if (ec) {
        log_error("Connection error: " + ec.message(), context);
    }
}

void WebSocketServer::handle_message_error(beast::error_code ec, const std::string& context) {
    if (ec) {
        log_error("Message error: " + ec.message(), context);
    }
}

void WebSocketServer::handle_subscription_error(const std::string& error_message, const std::string& context) {
    log_error("Subscription error: " + error_message, context);
}

void WebSocketServer::start() {
    if (running_) return;
    running_ = true;
    
    log_info("Starting server", "start");
    
    try {
        // Start worker threads for message processing
        for (unsigned int i = 0; i < std::thread::hardware_concurrency(); ++i) {
            worker_threads_.emplace_back([this]() {
                while (running_) {
                    process_messages();
                }
            });
        }
        
        // Start accepting connections
        accept();
        
        // Run the I/O context
        ioc_.run();
    } catch (const std::exception& e) {
        log_error("Failed to start server: " + std::string(e.what()), "start");
        stop();
        throw;
    }
}

void WebSocketServer::stop() {
    if (!running_) return;
    running_ = false;
    
    log_info("Stopping server", "stop");
    
    try {
        // Stop accepting new connections
        acceptor_.close();
        
        // Notify all worker threads
        queue_condition_.notify_all();
        
        // Join all worker threads
        for (auto& thread : worker_threads_) {
            if (thread.joinable()) {
                thread.join();
            }
        }
        
        // Clear subscriptions
        {
            std::lock_guard<std::mutex> lock(subscription_mutex_);
            subscriptions_.clear();
        }
    } catch (const std::exception& e) {
        log_error("Error during server shutdown: " + std::string(e.what()), "stop");
    }
}

void WebSocketServer::accept() {
    auto ws = std::make_shared<beast::websocket::stream<tcp::socket>>(ioc_);
    
    acceptor_.async_accept(
        ws->next_layer(),
        [this, ws](beast::error_code ec) {
            if (!ec) {
                handle_connection(ws);
            }
            if (running_) {
                accept();
            }
        });
}

void WebSocketServer::handle_connection(std::shared_ptr<beast::websocket::stream<tcp::socket>> ws) {
    ws->async_accept(
        [this, ws](beast::error_code ec) {
            if (!ec) {
                // Start reading messages from this client
                ws->async_read(
                    buffer_,
                    [this, ws](beast::error_code ec, std::size_t bytes_transferred) {
                        if (!ec) {
                            std::string message = beast::buffers_to_string(buffer_.data());
                            buffer_.consume(buffer_.size());
                            
                            try {
                                json json_message = json::parse(message);
                                handle_subscription(json_message, ws);
                            } catch (const std::exception& e) {
                                std::cerr << "Error parsing message: " << e.what() << std::endl;
                            }
                            
                            // Continue reading
                            handle_connection(ws);
                        }
                    });
            }
        });
}

void WebSocketServer::handle_subscription(const json& message, std::shared_ptr<beast::websocket::stream<tcp::socket>> ws) {
    if (message.contains("action") && message["action"] == "subscribe") {
        if (message.contains("symbol")) {
            std::string symbol = message["symbol"];
            subscribe(symbol, ws);
        }
    } else if (message.contains("action") && message["action"] == "unsubscribe") {
        if (message.contains("symbol")) {
            std::string symbol = message["symbol"];
            unsubscribe(symbol, ws);
        }
    }
}

void WebSocketServer::subscribe(const std::string& symbol, const std::shared_ptr<beast::websocket::stream<tcp::socket>>& client) {
    std::lock_guard<std::mutex> lock(subscription_mutex_);
    subscriptions_[symbol].insert(client);
}

void WebSocketServer::unsubscribe(const std::string& symbol, const std::shared_ptr<beast::websocket::stream<tcp::socket>>& client) {
    std::lock_guard<std::mutex> lock(subscription_mutex_);
    auto it = subscriptions_.find(symbol);
    if (it != subscriptions_.end()) {
        it->second.erase(client);
        if (it->second.empty()) {
            subscriptions_.erase(it);
        }
    }
}

void WebSocketServer::broadcast(const json& message) {
    {
        std::lock_guard<std::mutex> lock(queue_mutex_);
        message_queue_.push(message);
    }
    queue_condition_.notify_one();
}

void WebSocketServer::process_messages() {
    std::unique_lock<std::mutex> lock(queue_mutex_);
    queue_condition_.wait(lock, [this]() { return !message_queue_.empty() || !running_; });
    
    if (!running_) return;
    
    json message = message_queue_.front();
    message_queue_.pop();
    lock.unlock();
    
    std::string message_str = message.dump();
    
    std::lock_guard<std::mutex> sub_lock(subscription_mutex_);
    for (const auto& [symbol, clients] : subscriptions_) {
        for (const auto& client : clients) {
            client->async_write(
                asio::buffer(message_str),
                [](beast::error_code ec, std::size_t) {
                    if (ec) {
                        std::cerr << "Error broadcasting message: " << ec.message() << std::endl;
                    }
                });
        }
    }
} 