#include "config_manager.h"
#include <fstream>
#include <stdexcept>

ConfigManager::ConfigManager() {
    loadDefaultConfig();
}

void ConfigManager::loadDefaultConfig() {
    // Trading configuration
    trading_config_ = {
        100.0,      // max_position_size
        10.0,       // max_order_size
        1000.0,     // max_loss_per_trade
        5000.0,     // max_daily_loss
        10,         // max_open_orders
        0.001,      // slippage_tolerance
        0.0005,     // price_tolerance
        3,          // max_retries
        1000        // retry_delay_ms
    };

    // Network configuration
    network_config_ = {
        "https://test.deribit.com/api/v2",  // api_endpoint
        "wss://test.deribit.com/ws/api/v2", // websocket_endpoint
        5000,       // connection_timeout_ms
        3000,       // read_timeout_ms
        3000,       // write_timeout_ms
        30000,      // heartbeat_interval_ms
        1000,       // reconnect_interval_ms
        5           // max_reconnect_attempts
    };

    // Performance configuration
    performance_config_ = {
        100,        // latency_threshold_ms
        1024,       // memory_threshold_mb
        80,         // cpu_threshold_percent
        10000,      // max_queue_size
        100,        // batch_size
        1000        // flush_interval_ms
    };
}

bool ConfigManager::loadConfig(const std::string& config_file) {
    std::lock_guard<std::mutex> lock(config_mutex_);
    
    try {
        std::ifstream file(config_file);
        if (!file.is_open()) {
            return false;
        }

        nlohmann::json j;
        file >> j;

        // Load trading config
        if (j.contains("trading")) {
            const auto& trading = j["trading"];
            trading_config_.max_position_size = trading["max_position_size"];
            trading_config_.max_order_size = trading["max_order_size"];
            trading_config_.max_loss_per_trade = trading["max_loss_per_trade"];
            trading_config_.max_daily_loss = trading["max_daily_loss"];
            trading_config_.max_open_orders = trading["max_open_orders"];
            trading_config_.slippage_tolerance = trading["slippage_tolerance"];
            trading_config_.price_tolerance = trading["price_tolerance"];
            trading_config_.max_retries = trading["max_retries"];
            trading_config_.retry_delay_ms = trading["retry_delay_ms"];
        }

        // Load network config
        if (j.contains("network")) {
            const auto& network = j["network"];
            network_config_.api_endpoint = network["api_endpoint"];
            network_config_.websocket_endpoint = network["websocket_endpoint"];
            network_config_.connection_timeout_ms = network["connection_timeout_ms"];
            network_config_.read_timeout_ms = network["read_timeout_ms"];
            network_config_.write_timeout_ms = network["write_timeout_ms"];
            network_config_.heartbeat_interval_ms = network["heartbeat_interval_ms"];
            network_config_.reconnect_interval_ms = network["reconnect_interval_ms"];
            network_config_.max_reconnect_attempts = network["max_reconnect_attempts"];
        }

        // Load performance config
        if (j.contains("performance")) {
            const auto& performance = j["performance"];
            performance_config_.latency_threshold_ms = performance["latency_threshold_ms"];
            performance_config_.memory_threshold_mb = performance["memory_threshold_mb"];
            performance_config_.cpu_threshold_percent = performance["cpu_threshold_percent"];
            performance_config_.max_queue_size = performance["max_queue_size"];
            performance_config_.batch_size = performance["batch_size"];
            performance_config_.flush_interval_ms = performance["flush_interval_ms"];
        }

        validateConfig();
        return true;
    } catch (const std::exception& e) {
        return false;
    }
}

bool ConfigManager::saveConfig(const std::string& config_file) {
    std::lock_guard<std::mutex> lock(config_mutex_);
    
    try {
        nlohmann::json j;

        // Save trading config
        j["trading"] = {
            {"max_position_size", trading_config_.max_position_size},
            {"max_order_size", trading_config_.max_order_size},
            {"max_loss_per_trade", trading_config_.max_loss_per_trade},
            {"max_daily_loss", trading_config_.max_daily_loss},
            {"max_open_orders", trading_config_.max_open_orders},
            {"slippage_tolerance", trading_config_.slippage_tolerance},
            {"price_tolerance", trading_config_.price_tolerance},
            {"max_retries", trading_config_.max_retries},
            {"retry_delay_ms", trading_config_.retry_delay_ms}
        };

        // Save network config
        j["network"] = {
            {"api_endpoint", network_config_.api_endpoint},
            {"websocket_endpoint", network_config_.websocket_endpoint},
            {"connection_timeout_ms", network_config_.connection_timeout_ms},
            {"read_timeout_ms", network_config_.read_timeout_ms},
            {"write_timeout_ms", network_config_.write_timeout_ms},
            {"heartbeat_interval_ms", network_config_.heartbeat_interval_ms},
            {"reconnect_interval_ms", network_config_.reconnect_interval_ms},
            {"max_reconnect_attempts", network_config_.max_reconnect_attempts}
        };

        // Save performance config
        j["performance"] = {
            {"latency_threshold_ms", performance_config_.latency_threshold_ms},
            {"memory_threshold_mb", performance_config_.memory_threshold_mb},
            {"cpu_threshold_percent", performance_config_.cpu_threshold_percent},
            {"max_queue_size", performance_config_.max_queue_size},
            {"batch_size", performance_config_.batch_size},
            {"flush_interval_ms", performance_config_.flush_interval_ms}
        };

        std::ofstream file(config_file);
        if (!file.is_open()) {
            return false;
        }

        file << j.dump(4);
        return true;
    } catch (const std::exception& e) {
        return false;
    }
}

void ConfigManager::setTradingConfig(const TradingConfig& config) {
    std::lock_guard<std::mutex> lock(config_mutex_);
    trading_config_ = config;
    validateConfig();
}

void ConfigManager::setNetworkConfig(const NetworkConfig& config) {
    std::lock_guard<std::mutex> lock(config_mutex_);
    network_config_ = config;
    validateConfig();
}

void ConfigManager::setPerformanceConfig(const PerformanceConfig& config) {
    std::lock_guard<std::mutex> lock(config_mutex_);
    performance_config_ = config;
    validateConfig();
}

void ConfigManager::validateConfig() {
    // Validate trading config
    if (trading_config_.max_position_size <= 0 ||
        trading_config_.max_order_size <= 0 ||
        trading_config_.max_loss_per_trade <= 0 ||
        trading_config_.max_daily_loss <= 0 ||
        trading_config_.max_open_orders <= 0 ||
        trading_config_.slippage_tolerance <= 0 ||
        trading_config_.price_tolerance <= 0 ||
        trading_config_.max_retries < 0 ||
        trading_config_.retry_delay_ms < 0) {
        throw std::invalid_argument("Invalid trading configuration");
    }

    // Validate network config
    if (network_config_.connection_timeout_ms <= 0 ||
        network_config_.read_timeout_ms <= 0 ||
        network_config_.write_timeout_ms <= 0 ||
        network_config_.heartbeat_interval_ms <= 0 ||
        network_config_.reconnect_interval_ms <= 0 ||
        network_config_.max_reconnect_attempts < 0) {
        throw std::invalid_argument("Invalid network configuration");
    }

    // Validate performance config
    if (performance_config_.latency_threshold_ms <= 0 ||
        performance_config_.memory_threshold_mb <= 0 ||
        performance_config_.cpu_threshold_percent <= 0 ||
        performance_config_.max_queue_size <= 0 ||
        performance_config_.batch_size <= 0 ||
        performance_config_.flush_interval_ms <= 0) {
        throw std::invalid_argument("Invalid performance configuration");
    }
} 