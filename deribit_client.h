#ifndef DERIBIT_CLIENT_H
#define DERIBIT_CLIENT_H

#include <string>
#include <map>
#include <functional>
#include <memory>
#include <chrono>
#include "config_manager.h"
#include "market_data_manager.h"

class DeribitClient {
public:
    enum class InstrumentType {
        SPOT,
        FUTURE,
        OPTION
    };

    struct InstrumentInfo {
        std::string name;
        InstrumentType type;
        double tick_size;
        double min_order_size;
        double max_order_size;
        double contract_size;
        std::string base_currency;
        std::string quote_currency;
        std::string settlement_currency;
        std::chrono::system_clock::time_point expiry;
    };

    struct OrderRequest {
        std::string instrument;
        std::string side;
        double size;
        double price;
        std::string type;  // "limit", "market", "stop_limit", etc.
        bool post_only;
        bool reduce_only;
        std::string time_in_force;  // "good_til_cancelled", "fill_or_kill", etc.
        double stop_price;  // For stop orders
        double trigger_price;  // For trigger orders
        bool iceberg;  // For iceberg orders
        double visible_size;  // For iceberg orders
    };

    struct Order {
        std::string order_id;
        std::string instrument;
        std::string side;
        double size;
        double price;
        std::string type;
        std::string status;
        std::chrono::system_clock::time_point timestamp;
        double filled_size;
        double average_price;
        double commission;
        double stop_price;
        double trigger_price;
        bool iceberg;
        double visible_size;
    };

    struct Position {
        std::string instrument;
        double size;
        double entry_price;
        double mark_price;
        double liquidation_price;
        double unrealized_pnl;
        double realized_pnl;
        std::chrono::system_clock::time_point timestamp;
        double initial_margin;
        double maintenance_margin;
        double leverage;
        double delta;
        double gamma;
        double theta;
        double vega;
    };

    static DeribitClient& getInstance() {
        static DeribitClient instance;
        return instance;
    }

    void initialize(const std::string& api_key, const std::string& api_secret);
    void shutdown();

    // Authentication
    bool authenticate();
    void refreshToken();

    // Instrument Management
    InstrumentInfo getInstrumentInfo(const std::string& instrument);
    std::vector<InstrumentInfo> getAllInstruments();
    std::vector<InstrumentInfo> getInstrumentsByType(InstrumentType type);
    std::vector<InstrumentInfo> getInstrumentsByCurrency(const std::string& currency);

    // Order Management
    std::string placeOrder(const OrderRequest& request);
    bool cancelOrder(const std::string& order_id);
    bool modifyOrder(const std::string& order_id, double new_size, double new_price);
    Order getOrder(const std::string& order_id);
    std::vector<Order> getOpenOrders(const std::string& instrument = "");
    std::vector<Order> getOrderHistory(const std::string& instrument = "", 
                                     const std::chrono::system_clock::time_point& start_time = std::chrono::system_clock::now() - std::chrono::hours(24),
                                     const std::chrono::system_clock::time_point& end_time = std::chrono::system_clock::now());

    // Position Management
    Position getPosition(const std::string& instrument);
    std::vector<Position> getAllPositions();
    bool closePosition(const std::string& instrument);
    bool setLeverage(const std::string& instrument, double leverage);
    
    // Market Data
    MarketDataManager::OrderBook getOrderBook(const std::string& instrument);
    double getMarkPrice(const std::string& instrument);
    double getIndexPrice(const std::string& instrument);
    double getLastPrice(const std::string& instrument);
    double getFundingRate(const std::string& instrument);
    std::vector<MarketDataManager::Trade> getRecentTrades(const std::string& instrument, int limit = 100);

    // WebSocket Management
    void subscribeToOrderBook(const std::string& instrument);
    void subscribeToTrades(const std::string& instrument);
    void subscribeToUserData();
    void subscribeToInstrumentUpdates();
    void unsubscribe(const std::string& channel);

    // Callbacks
    void setOrderCallback(std::function<void(const Order&)> callback);
    void setPositionCallback(std::function<void(const Position&)> callback);
    void setErrorCallback(std::function<void(const std::string&)> callback);
    void setInstrumentCallback(std::function<void(const InstrumentInfo&)> callback);

private:
    DeribitClient();
    ~DeribitClient();
    DeribitClient(const DeribitClient&) = delete;
    DeribitClient& operator=(const DeribitClient&) = delete;

    void handleWebSocketMessage(const std::string& message);
    void processOrderBookUpdate(const std::string& instrument, const nlohmann::json& data);
    void processTradeUpdate(const std::string& instrument, const nlohmann::json& data);
    void processUserDataUpdate(const nlohmann::json& data);
    void processInstrumentUpdate(const nlohmann::json& data);
    void reconnectWebSocket();
    void updateInstrumentCache();
    InstrumentType parseInstrumentType(const std::string& instrument_name);

    std::string api_key_;
    std::string api_secret_;
    std::string access_token_;
    std::string refresh_token_;
    std::chrono::system_clock::time_point token_expiry_;
    bool is_connected_;
    std::unique_ptr<WebSocket> websocket_;
    std::function<void(const Order&)> order_callback_;
    std::function<void(const Position&)> position_callback_;
    std::function<void(const std::string&)> error_callback_;
    std::function<void(const InstrumentInfo&)> instrument_callback_;
    const ConfigManager& config_manager_;
    MarketDataManager& market_data_manager_;
    std::map<std::string, InstrumentInfo> instrument_cache_;
    std::chrono::system_clock::time_point last_instrument_update_;
};

#endif // DERIBIT_CLIENT_H 