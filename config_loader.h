#pragma once

#include <string>
#include <memory>
#include <nlohmann/json.hpp>

class ConfigLoader {
public:
    static ConfigLoader& getInstance();
    
    void loadConfig(const std::string& configPath);
    void validateConfig();
    
    // API settings
    std::string getApiKey() const;
    std::string getApiSecret() const;
    bool isTestMode() const;
    std::string getWsUrl() const;
    
    // Trading settings
    std::vector<std::string> getInstruments() const;
    double getMaxPositionSize() const;
    int getMaxLeverage() const;
    double getRiskLimitPct() const;
    double getStopLossPct() const;
    double getTakeProfitPct() const;
    
    // Execution settings
    std::string getOrderType() const;
    bool isPostOnly() const;
    std::string getTimeInForce() const;
    int getMaxRetryAttempts() const;
    int getRetryDelayMs() const;
    
    // Performance settings
    int getMaxLatencyMs() const;
    bool shouldLogPerformanceStats() const;
    int getStatsIntervalSec() const;
    int getMemoryLimitMb() const;
    
    // Logging settings
    std::string getLogLevel() const;
    bool shouldLogToFile() const;
    std::string getLogDirectory() const;
    int getMaxLogFiles() const;
    int getMaxFileSizeMb() const;

private:
    ConfigLoader() = default;
    ~ConfigLoader() = default;
    ConfigLoader(const ConfigLoader&) = delete;
    ConfigLoader& operator=(const ConfigLoader&) = delete;

    nlohmann::json config_;
    
    void validateApiSettings();
    void validateTradingSettings();
    void validateExecutionSettings();
    void validatePerformanceSettings();
    void validateLoggingSettings();
}; 