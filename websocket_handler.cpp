#include "websocket_handler.h"
#include <iostream> // For debugging (optional)
#include "latency_module.h"  // Include the LatencyModule header

WebSocketHandler::WebSocketHandler(const std::string& host, const std::string& port, const std::string& endpoint )
    : ctx_(ssl::context::tlsv12_client),
    resolver_(ioc_),
    websocket_(ioc_, ctx_),
    host_(host),
    endpoint_(endpoint) {
    //trade_execution_(trade_execution) {  // Initialize the TradeExecution reference
    // Load the default SSL certificates
    ctx_.set_default_verify_paths();
}

void WebSocketHandler::connect() {
    try {
        // Resolve the host and port
        auto const results = resolver_.resolve(host_, "443");

        // Connect to the server
        asio::connect(websocket_.next_layer().next_layer(), results.begin(), results.end());

        // Perform the SSL handshake
        websocket_.next_layer().handshake(ssl::stream_base::client);

        // Perform the WebSocket handshake
        websocket_.handshake(host_, endpoint_);

        std::cout << "WebSocket connected successfully!" << std::endl;
    }
    catch (const std::exception& e) {
        std::cerr << "Error during WebSocket connection: " << e.what() << std::endl;
    }
}

void WebSocketHandler::onMessage(const std::string& message) {
    // Parse the incoming message into JSON
    json data = json::parse(message);


}

void WebSocketHandler::sendMessage(const json& message) {
    try {
        // Serialize the JSON message and send it
        std::string message_str = message.dump();
        websocket_.write(asio::buffer(message_str));

        std::cout << "Sent message: " << message_str << std::endl;
    }
    catch (const std::exception& e) {
        std::cerr << "Error sending message: " << e.what() << std::endl;
    }
}

json WebSocketHandler::readMessage() {
    try {
        auto read_start = LatencyModule::start();  // Start timer for WebSocket message read

        beast::flat_buffer buffer;
        websocket_.read(buffer);

        // Parse the received message as JSON
        std::string message_str = beast::buffers_to_string(buffer.data());
        std::cout << "Received message: " << message_str << std::endl;

        // End the timer and log the latency
        LatencyModule::end(read_start, "WebSocket Read Latency");

        return json::parse(message_str);
    }
    catch (const std::exception& e) {
        std::cerr << "Error reading message: " << e.what() << std::endl;
        return json();  // Return an empty JSON object in case of error
    }
}

void WebSocketHandler::close() {
    try {
        websocket_.close(beast::websocket::close_code::normal);
        std::cout << "WebSocket connection closed." << std::endl;
    }
    catch (const std::exception& e) {
        std::cerr << "Error closing WebSocket: " << e.what() << std::endl;
    }
}
