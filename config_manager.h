#ifndef CONFIG_MANAGER_H
#define CONFIG_MANAGER_H

#include <string>
#include <map>
#include <mutex>
#include <memory>
#include <fstream>
#include <nlohmann/json.hpp>

class ConfigManager {
public:
    struct TradingConfig {
        double max_position_size;
        double max_order_size;
        double max_loss_per_trade;
        double max_daily_loss;
        int max_open_orders;
        double slippage_tolerance;
        double price_tolerance;
        int max_retries;
        int retry_delay_ms;
    };

    struct NetworkConfig {
        std::string api_endpoint;
        std::string websocket_endpoint;
        int connection_timeout_ms;
        int read_timeout_ms;
        int write_timeout_ms;
        int heartbeat_interval_ms;
        int reconnect_interval_ms;
        int max_reconnect_attempts;
    };

    struct PerformanceConfig {
        int latency_threshold_ms;
        int memory_threshold_mb;
        int cpu_threshold_percent;
        int max_queue_size;
        int batch_size;
        int flush_interval_ms;
    };

    static ConfigManager& getInstance() {
        static ConfigManager instance;
        return instance;
    }

    bool loadConfig(const std::string& config_file);
    bool saveConfig(const std::string& config_file);
    
    const TradingConfig& getTradingConfig() const { return trading_config_; }
    const NetworkConfig& getNetworkConfig() const { return network_config_; }
    const PerformanceConfig& getPerformanceConfig() const { return performance_config_; }

    void setTradingConfig(const TradingConfig& config);
    void setNetworkConfig(const NetworkConfig& config);
    void setPerformanceConfig(const PerformanceConfig& config);

private:
    ConfigManager();
    ~ConfigManager() = default;
    ConfigManager(const ConfigManager&) = delete;
    ConfigManager& operator=(const ConfigManager&) = delete;

    void loadDefaultConfig();
    void validateConfig();

    TradingConfig trading_config_;
    NetworkConfig network_config_;
    PerformanceConfig performance_config_;
    std::mutex config_mutex_;
};

#endif // CONFIG_MANAGER_H 