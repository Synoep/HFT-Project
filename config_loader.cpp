#include "config_loader.h"
#include <fstream>
#include <stdexcept>
#include <filesystem>

ConfigLoader& ConfigLoader::getInstance() {
    static ConfigLoader instance;
    return instance;
}

void ConfigLoader::loadConfig(const std::string& configPath) {
    if (!std::filesystem::exists(configPath)) {
        throw std::runtime_error("Configuration file not found: " + configPath);
    }

    std::ifstream configFile(configPath);
    if (!configFile.is_open()) {
        throw std::runtime_error("Failed to open configuration file: " + configPath);
    }

    try {
        configFile >> config_;
    } catch (const nlohmann::json::parse_error& e) {
        throw std::runtime_error("Failed to parse configuration file: " + std::string(e.what()));
    }

    validateConfig();
}

void ConfigLoader::validateConfig() {
    validateApiSettings();
    validateTradingSettings();
    validateExecutionSettings();
    validatePerformanceSettings();
    validateLoggingSettings();
}

void ConfigLoader::validateApiSettings() {
    if (!config_.contains("api")) {
        throw std::runtime_error("Missing 'api' section in config");
    }
    
    const auto& api = config_["api"];
    if (!api.contains("key") || !api.contains("secret") || !api.contains("test_mode")) {
        throw std::runtime_error("Missing required API settings");
    }
}

void ConfigLoader::validateTradingSettings() {
    if (!config_.contains("trading")) {
        throw std::runtime_error("Missing 'trading' section in config");
    }
    
    const auto& trading = config_["trading"];
    if (!trading.contains("instruments") || !trading.contains("max_position_size") ||
        !trading.contains("max_leverage") || !trading.contains("risk_limit_pct") ||
        !trading.contains("stop_loss_pct") || !trading.contains("take_profit_pct")) {
        throw std::runtime_error("Missing required trading settings");
    }
}

void ConfigLoader::validateExecutionSettings() {
    if (!config_.contains("execution")) {
        throw std::runtime_error("Missing 'execution' section in config");
    }
    
    const auto& execution = config_["execution"];
    if (!execution.contains("order_type") || !execution.contains("post_only") ||
        !execution.contains("time_in_force") || !execution.contains("max_retry_attempts") ||
        !execution.contains("retry_delay_ms")) {
        throw std::runtime_error("Missing required execution settings");
    }
}

void ConfigLoader::validatePerformanceSettings() {
    if (!config_.contains("performance")) {
        throw std::runtime_error("Missing 'performance' section in config");
    }
    
    const auto& performance = config_["performance"];
    if (!performance.contains("max_latency_ms") || !performance.contains("log_performance_stats") ||
        !performance.contains("stats_interval_sec") || !performance.contains("memory_limit_mb")) {
        throw std::runtime_error("Missing required performance settings");
    }
}

void ConfigLoader::validateLoggingSettings() {
    if (!config_.contains("logging")) {
        throw std::runtime_error("Missing 'logging' section in config");
    }
    
    const auto& logging = config_["logging"];
    if (!logging.contains("log_level") || !logging.contains("log_to_file") ||
        !logging.contains("log_directory") || !logging.contains("max_log_files") ||
        !logging.contains("max_file_size_mb")) {
        throw std::runtime_error("Missing required logging settings");
    }
}

// Getter implementations
std::string ConfigLoader::getApiKey() const {
    return config_["api"]["key"];
}

std::string ConfigLoader::getApiSecret() const {
    return config_["api"]["secret"];
}

bool ConfigLoader::isTestMode() const {
    return config_["api"]["test_mode"];
}

std::string ConfigLoader::getWsUrl() const {
    return isTestMode() ? config_["api"]["test_ws_url"] : config_["api"]["prod_ws_url"];
}

std::vector<std::string> ConfigLoader::getInstruments() const {
    return config_["trading"]["instruments"].get<std::vector<std::string>>();
}

double ConfigLoader::getMaxPositionSize() const {
    return config_["trading"]["max_position_size"];
}

int ConfigLoader::getMaxLeverage() const {
    return config_["trading"]["max_leverage"];
}

double ConfigLoader::getRiskLimitPct() const {
    return config_["trading"]["risk_limit_pct"];
}

double ConfigLoader::getStopLossPct() const {
    return config_["trading"]["stop_loss_pct"];
}

double ConfigLoader::getTakeProfitPct() const {
    return config_["trading"]["take_profit_pct"];
}

std::string ConfigLoader::getOrderType() const {
    return config_["execution"]["order_type"];
}

bool ConfigLoader::isPostOnly() const {
    return config_["execution"]["post_only"];
}

std::string ConfigLoader::getTimeInForce() const {
    return config_["execution"]["time_in_force"];
}

int ConfigLoader::getMaxRetryAttempts() const {
    return config_["execution"]["max_retry_attempts"];
}

int ConfigLoader::getRetryDelayMs() const {
    return config_["execution"]["retry_delay_ms"];
}

int ConfigLoader::getMaxLatencyMs() const {
    return config_["performance"]["max_latency_ms"];
}

bool ConfigLoader::shouldLogPerformanceStats() const {
    return config_["performance"]["log_performance_stats"];
}

int ConfigLoader::getStatsIntervalSec() const {
    return config_["performance"]["stats_interval_sec"];
}

int ConfigLoader::getMemoryLimitMb() const {
    return config_["performance"]["memory_limit_mb"];
}

std::string ConfigLoader::getLogLevel() const {
    return config_["logging"]["log_level"];
}

bool ConfigLoader::shouldLogToFile() const {
    return config_["logging"]["log_to_file"];
}

std::string ConfigLoader::getLogDirectory() const {
    return config_["logging"]["log_directory"];
}

int ConfigLoader::getMaxLogFiles() const {
    return config_["logging"]["max_log_files"];
}

int ConfigLoader::getMaxFileSizeMb() const {
    return config_["logging"]["max_file_size_mb"];
} 