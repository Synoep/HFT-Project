#pragma once
#ifndef WEBSOCKET_SERVER_H
#define WEBSOCKET_SERVER_H

#include <boost/beast/core.hpp>
#include <boost/beast/websocket.hpp>
#include <boost/asio/ip/tcp.hpp>
#include <boost/asio/ssl.hpp>
#include <nlohmann/json.hpp>
#include <memory>
#include <string>
#include <thread>
#include <mutex>
#include <queue>
#include <condition_variable>
#include <unordered_map>
#include <set>
#include <fstream>
#include <chrono>

namespace beast = boost::beast;
namespace asio = boost::asio;
namespace ssl = boost::asio::ssl;
using tcp = boost::asio::ip::tcp;
using json = nlohmann::json;

class WebSocketServer {
public:
    WebSocketServer(const std::string& host, const std::string& port);
    ~WebSocketServer();

    // Prevent copying and moving
    WebSocketServer(const WebSocketServer&) = delete;
    WebSocketServer& operator=(const WebSocketServer&) = delete;
    WebSocketServer(WebSocketServer&&) = delete;
    WebSocketServer& operator=(WebSocketServer&&) = delete;

    void start();
    void stop();
    void broadcast(const json& message);
    void subscribe(const std::string& symbol, const std::shared_ptr<beast::websocket::stream<tcp::socket>>& client);
    void unsubscribe(const std::string& symbol, const std::shared_ptr<beast::websocket::stream<tcp::socket>>& client);

private:
    void accept();
    void handle_connection(std::shared_ptr<beast::websocket::stream<tcp::socket>> ws);
    void handle_subscription(const json& message, std::shared_ptr<beast::websocket::stream<tcp::socket>> ws);
    void process_messages();
    void log_error(const std::string& error_message, const std::string& context);
    void log_info(const std::string& info_message, const std::string& context);
    void handle_connection_error(beast::error_code ec, const std::string& context);
    void handle_message_error(beast::error_code ec, const std::string& context);
    void handle_subscription_error(const std::string& error_message, const std::string& context);

    std::string host_;
    std::string port_;
    asio::io_context ioc_;
    tcp::acceptor acceptor_;
    std::vector<std::thread> worker_threads_;
    std::thread io_thread_;
    std::atomic<bool> running_{false};
    std::mutex queue_mutex_;
    std::condition_variable queue_condition_;
    std::queue<json> message_queue_;
    std::mutex subscription_mutex_;
    std::unordered_map<std::string, std::set<std::shared_ptr<beast::websocket::stream<tcp::socket>>>> subscriptions_;
    beast::flat_buffer buffer_;
    std::ofstream error_log_;
    std::ofstream info_log_;
    std::chrono::steady_clock::time_point start_time_;
};

#endif // WEBSOCKET_SERVER_H 