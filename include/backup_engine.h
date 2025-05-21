#pragma once

#include <string>
#include <memory>
#include <chrono>
#include <nlohmann/json.hpp>
#include "nfs_handler.h"
#include "logger.h"

using json = nlohmann::json;

class BackupEngine {
private:
    std::shared_ptr<NFSHandler> nfs_handler_;
    std::shared_ptr<Logger> logger_;
    json config_;
    std::chrono::system_clock::time_point last_backup_time_;
    int retention_days_;

    bool checkRetentionPolicy();
    bool cleanupOldBackups();
    
    // JSON validation helpers
    bool validateConfig(const json& config);
    std::string getRequiredString(const json& config, const std::string& key);
    int getRequiredInt(const json& config, const std::string& key);

public:
    BackupEngine(const std::string& config_path, std::shared_ptr<Logger> logger);
    ~BackupEngine() = default;

    // Core backup operations
    bool initialize();
    bool performBackup(const std::string& source_path);
    bool verifyBackup(const std::string& source_path);
    
    // Backup management
    bool scheduleBackup(const std::string& source_path, 
                       const std::chrono::system_clock::duration& interval);
    bool cleanup();
    
    // Status and information
    std::chrono::system_clock::time_point getLastBackupTime() const { return last_backup_time_; }
    int getRetentionDays() const { return retention_days_; }
    
    // Configuration access
    const json& getConfig() const { return config_; }
};
