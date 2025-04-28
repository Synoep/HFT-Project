#pragma once
#ifndef WEBSOCKET_HANDLER_H
#define WEBSOCKET_HANDLER_H

#include <boost/beast.hpp>
#include <boost/asio.hpp>
#include <boost/asio/ssl.hpp>
#include <nlohmann/json.hpp>
#include <openssl/ssl.h>
#include <boost/beast/websocket.hpp>
#include <boost/beast/ssl.hpp>
#include <boost/beast/core.hpp>
#include <string>
#include "trade_execution.h"  // Include the TradeExecution header for access

namespace beast = boost::beast;
namespace asio = boost::asio;
namespace ssl = asio::ssl;
using tcp = asio::ip::tcp;
using json = nlohmann::json;

class WebSocketHandler {
public:
    // Constructor now includes TradeExecution reference
    WebSocketHandler(const std::string& host, const std::string& port, const std::string& endpoint );

    void connect();
    void onMessage(const std::string& message); // Declare the onMessage function
    void sendMessage(const json& message);
    json readMessage();
    void close();

private:
    asio::io_context ioc_;
    ssl::context ctx_;
    tcp::resolver resolver_;
    beast::websocket::stream<ssl::stream<tcp::socket>> websocket_;
    std::string host_;
    std::string endpoint_;
    // TradeExecution& trade_execution_;  // Reference to TradeExecution object
};

#endif // WEBSOCKET_HANDLER_H
