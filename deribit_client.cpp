#include "deribit_client.h"
#include <cpprest/http_client.h>
#include <cpprest/ws_client.h>
#include <openssl/hmac.h>
#include <openssl/sha.h>
#include <sstream>
#include <iomanip>

using namespace web;
using namespace web::http;
using namespace web::http::client;
using namespace web::websockets::client;

DeribitClient::DeribitClient()
    : is_connected_(false),
      config_manager_(ConfigManager::getInstance()),
      market_data_manager_(MarketDataManager::getInstance()) {
}

DeribitClient::~DeribitClient() {
    shutdown();
}

void DeribitClient::initialize(const std::string& api_key, const std::string& api_secret) {
    api_key_ = api_key;
    api_secret_ = api_secret;
    
    // Initialize REST client
    const auto& network_config = config_manager_.getNetworkConfig();
    
    // Initialize WebSocket connection
    websocket_ = std::make_unique<websocket_callback_client>();
    
    websocket_->connect(network_config.websocket_endpoint).then([this]() {
        is_connected_ = true;
        authenticate();
    }).wait();
    
    // Set up WebSocket message handler
    websocket_->set_message_handler([this](const websocket_incoming_message& msg) {
        msg.extract_string().then([this](const std::string& str) {
            handleWebSocketMessage(str);
        });
    });
    
    // Set up WebSocket close handler
    websocket_->set_close_handler([this](websocket_close_status status, const std::string& reason) {
        is_connected_ = false;
        if (error_callback_) {
            error_callback_("WebSocket connection closed: " + reason);
        }
        reconnectWebSocket();
    });
}

void DeribitClient::shutdown() {
    if (is_connected_ && websocket_) {
        websocket_->close().wait();
    }
}

bool DeribitClient::authenticate() {
    if (!is_connected_) return false;
    
    // Create authentication message
    nlohmann::json auth_msg = {
        {"jsonrpc", "2.0"},
        {"id", 9929},
        {"method", "public/auth"},
        {"params", {
            {"grant_type", "client_credentials"},
            {"client_id", api_key_},
            {"client_secret", api_secret_}
        }}
    };
    
    // Send authentication message
    websocket_->send(auth_msg.dump()).wait();
    return true;
}

void DeribitClient::refreshToken() {
    if (!is_connected_) return;
    
    nlohmann::json refresh_msg = {
        {"jsonrpc", "2.0"},
        {"id", 9930},
        {"method", "public/auth"},
        {"params", {
            {"grant_type", "refresh_token"},
            {"refresh_token", refresh_token_}
        }}
    };
    
    websocket_->send(refresh_msg.dump()).wait();
}

std::string DeribitClient::placeOrder(const OrderRequest& request) {
    nlohmann::json order_msg = {
        {"jsonrpc", "2.0"},
        {"id", 9931},
        {"method", "private/buy" + (request.side == "sell" ? "private/sell" : "")},
        {"params", {
            {"instrument_name", request.instrument},
            {"amount", request.size},
            {"type", request.type},
            {"price", request.price},
            {"post_only", request.post_only},
            {"reduce_only", request.reduce_only},
            {"time_in_force", request.time_in_force}
        }}
    };
    
    websocket_->send(order_msg.dump()).wait();
    return "pending_order_id"; // The actual order ID will come in the response
}

bool DeribitClient::cancelOrder(const std::string& order_id) {
    nlohmann::json cancel_msg = {
        {"jsonrpc", "2.0"},
        {"id", 9932},
        {"method", "private/cancel"},
        {"params", {
            {"order_id", order_id}
        }}
    };
    
    websocket_->send(cancel_msg.dump()).wait();
    return true;
}

bool DeribitClient::modifyOrder(const std::string& order_id, double new_size, double new_price) {
    nlohmann::json modify_msg = {
        {"jsonrpc", "2.0"},
        {"id", 9933},
        {"method", "private/edit"},
        {"params", {
            {"order_id", order_id},
            {"amount", new_size},
            {"price", new_price}
        }}
    };
    
    websocket_->send(modify_msg.dump()).wait();
    return true;
}

void DeribitClient::subscribeToOrderBook(const std::string& instrument) {
    nlohmann::json sub_msg = {
        {"jsonrpc", "2.0"},
        {"id", 9934},
        {"method", "public/subscribe"},
        {"params", {
            {"channels", {"book." + instrument + ".100ms"}}
        }}
    };
    
    websocket_->send(sub_msg.dump()).wait();
}

void DeribitClient::subscribeToTrades(const std::string& instrument) {
    nlohmann::json sub_msg = {
        {"jsonrpc", "2.0"},
        {"id", 9935},
        {"method", "public/subscribe"},
        {"params", {
            {"channels", {"trades." + instrument + ".100ms"}}
        }}
    };
    
    websocket_->send(sub_msg.dump()).wait();
}

void DeribitClient::subscribeToUserData() {
    nlohmann::json sub_msg = {
        {"jsonrpc", "2.0"},
        {"id", 9936},
        {"method", "private/subscribe"},
        {"params", {
            {"channels", {"user.orders.*", "user.trades.*", "user.portfolio.*"}}
        }}
    };
    
    websocket_->send(sub_msg.dump()).wait();
}

void DeribitClient::handleWebSocketMessage(const std::string& message) {
    try {
        auto json = nlohmann::json::parse(message);
        
        // Handle subscription messages
        if (json.contains("method") && json["method"] == "subscription") {
            const auto& params = json["params"];
            const auto& channel = params["channel"].get<std::string>();
            
            if (channel.starts_with("book.")) {
                std::string instrument = channel.substr(5, channel.find(".", 5) - 5);
                processOrderBookUpdate(instrument, params["data"]);
            }
            else if (channel.starts_with("trades.")) {
                std::string instrument = channel.substr(7, channel.find(".", 7) - 7);
                processTradeUpdate(instrument, params["data"]);
            }
            else if (channel.starts_with("user.")) {
                processUserDataUpdate(params["data"]);
            }
        }
        // Handle response messages
        else if (json.contains("id")) {
            // Process specific responses based on the request ID
            // This would be expanded based on the actual response handling needs
        }
        
    } catch (const std::exception& e) {
        if (error_callback_) {
            error_callback_("Error processing WebSocket message: " + std::string(e.what()));
        }
    }
}

void DeribitClient::processOrderBookUpdate(const std::string& instrument, const nlohmann::json& data) {
    MarketDataManager::OrderBook orderbook;
    orderbook.instrument = instrument;
    orderbook.timestamp = std::chrono::system_clock::now();
    
    // Process bids
    for (const auto& bid : data["bids"]) {
        MarketDataManager::OrderBook::Level level;
        level.price = bid[0];
        level.size = bid[1];
        level.timestamp = orderbook.timestamp;
        orderbook.bids.push_back(level);
    }
    
    // Process asks
    for (const auto& ask : data["asks"]) {
        MarketDataManager::OrderBook::Level level;
        level.price = ask[0];
        level.size = ask[1];
        level.timestamp = orderbook.timestamp;
        orderbook.asks.push_back(level);
    }
    
    market_data_manager_.updateOrderBook(orderbook);
}

void DeribitClient::processTradeUpdate(const std::string& instrument, const nlohmann::json& data) {
    MarketDataManager::Trade trade;
    trade.instrument = instrument;
    trade.price = data["price"];
    trade.size = data["amount"];
    trade.side = data["direction"];
    trade.timestamp = std::chrono::system_clock::now();
    
    market_data_manager_.addTrade(trade);
}

void DeribitClient::processUserDataUpdate(const nlohmann::json& data) {
    if (data.contains("order")) {
        Order order;
        order.order_id = data["order"]["order_id"];
        order.instrument = data["order"]["instrument_name"];
        order.side = data["order"]["direction"];
        order.size = data["order"]["amount"];
        order.price = data["order"]["price"];
        order.type = data["order"]["order_type"];
        order.status = data["order"]["order_state"];
        order.timestamp = std::chrono::system_clock::now();
        
        if (order_callback_) {
            order_callback_(order);
        }
    }
    else if (data.contains("position")) {
        Position position;
        position.instrument = data["position"]["instrument_name"];
        position.size = data["position"]["size"];
        position.entry_price = data["position"]["average_price"];
        position.mark_price = data["position"]["mark_price"];
        position.liquidation_price = data["position"]["estimated_liquidation_price"];
        position.unrealized_pnl = data["position"]["floating_profit_loss"];
        position.realized_pnl = data["position"]["realized_profit_loss"];
        position.timestamp = std::chrono::system_clock::now();
        
        if (position_callback_) {
            position_callback_(position);
        }
    }
}

void DeribitClient::reconnectWebSocket() {
    const auto& network_config = config_manager_.getNetworkConfig();
    
    if (websocket_) {
        websocket_->close().wait();
    }
    
    websocket_ = std::make_unique<websocket_callback_client>();
    
    websocket_->connect(network_config.websocket_endpoint).then([this]() {
        is_connected_ = true;
        authenticate();
    }).wait();
} 