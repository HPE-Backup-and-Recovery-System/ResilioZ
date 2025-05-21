#include "backup_engine.h"
#include <fstream>
#include <filesystem>
#include <thread>
#include <chrono>

namespace fs = std::filesystem;

bool BackupEngine::validateConfig(const json& config) {
    try {
        // Check required fields
        if (!config.contains("nfs_server") || !config["nfs_server"].is_string()) {
            logger_->log("Config missing or invalid nfs_server", LogLevel::ERROR);
            return false;
        }
        if (!config.contains("mount_point") || !config["mount_point"].is_string()) {
            logger_->log("Config missing or invalid mount_point", LogLevel::ERROR);
            return false;
        }
        if (!config.contains("backup_command") || !config["backup_command"].is_string()) {
            logger_->log("Config missing or invalid backup_command", LogLevel::ERROR);
            return false;
        }
        if (!config.contains("retention_policy") || !config["retention_policy"].is_number_integer()) {
            logger_->log("Config missing or invalid retention_policy", LogLevel::ERROR);
            return false;
        }
        return true;
    } catch (const json::exception& e) {
        logger_->log("JSON validation error: " + std::string(e.what()), LogLevel::ERROR);
        return false;
    }
}

std::string BackupEngine::getRequiredString(const json& config, const std::string& key) {
    if (!config.contains(key) || !config[key].is_string()) {
        throw std::runtime_error("Missing or invalid string field: " + key);
    }
    return config[key].get<std::string>();
}

int BackupEngine::getRequiredInt(const json& config, const std::string& key) {
    if (!config.contains(key) || !config[key].is_number_integer()) {
        throw std::runtime_error("Missing or invalid integer field: " + key);
    }
    return config[key].get<int>();
}

BackupEngine::BackupEngine(const std::string& config_path, std::shared_ptr<Logger> logger)
    : logger_(logger), last_backup_time_(std::chrono::system_clock::now()) {
    try {
        // Read and parse config file
        std::ifstream f(config_path);
        if (!f.is_open()) {
            throw std::runtime_error("Could not open config file: " + config_path);
        }
        
        config_ = json::parse(f);
        
        // Validate config
        if (!validateConfig(config_)) {
            throw std::runtime_error("Invalid configuration file");
        }
        
        // Extract required values
        retention_days_ = getRequiredInt(config_, "retention_policy");
        
        // Initialize NFS handler
        nfs_handler_ = std::make_shared<NFSHandler>(config_path, logger_);
        
        logger_->log("Configuration loaded successfully", LogLevel::INFO);
    } catch (const json::parse_error& e) {
        logger_->log("JSON parse error: " + std::string(e.what()), LogLevel::ERROR);
        throw;
    } catch (const std::exception& e) {
        logger_->log("Error initializing backup engine: " + std::string(e.what()), LogLevel::ERROR);
        throw;
    }
}

bool BackupEngine::initialize() {
    try {
        if (!nfs_handler_->initialize()) {
            logger_->log("Failed to initialize NFS handler", LogLevel::ERROR);
            return false;
        }

        if (!nfs_handler_->mountNFSShare()) {
            logger_->log("Failed to mount NFS", LogLevel::ERROR);
            return false;
        }

        logger_->log("Backup engine initialized successfully");
        return true;
    } catch (const std::exception& e) {
        logger_->log("Error during initialization: " + std::string(e.what()), LogLevel::ERROR);
        return false;
    }
}

bool BackupEngine::performBackup(const std::string& source_path) {
    try {
        if (!fs::exists(source_path)) {
            logger_->log("Source path does not exist: " + source_path, LogLevel::ERROR);
            return false;
        }

        logger_->log("Starting backup from: " + source_path);
        
        if (!nfs_handler_->performBackup(source_path)) {
            logger_->log("Backup failed", LogLevel::ERROR);
            return false;
        }

        if (!nfs_handler_->verifyBackup(source_path)) {
            logger_->log("Backup verification failed", LogLevel::ERROR);
            return false;
        }

        last_backup_time_ = std::chrono::system_clock::now();
        logger_->log("Backup completed successfully");
        
        // Check and cleanup old backups
        if (checkRetentionPolicy()) {
            cleanupOldBackups();
        }

        return true;
    } catch (const std::exception& e) {
        logger_->log("Error during backup: " + std::string(e.what()), LogLevel::ERROR);
        return false;
    }
}

bool BackupEngine::verifyBackup(const std::string& source_path) {
    try {
        return nfs_handler_->verifyBackup(source_path);
    } catch (const std::exception& e) {
        logger_->log("Error during backup verification: " + std::string(e.what()), LogLevel::ERROR);
        return false;
    }
}

bool BackupEngine::scheduleBackup(const std::string& source_path, 
                                const std::chrono::system_clock::duration& interval) {
    try {
        while (true) {
            performBackup(source_path);
            std::this_thread::sleep_for(interval);
        }
        return true;
    } catch (const std::exception& e) {
        logger_->log("Error in backup schedule: " + std::string(e.what()), LogLevel::ERROR);
        return false;
    }
}

bool BackupEngine::checkRetentionPolicy() {
    auto now = std::chrono::system_clock::now();
    auto days_since_last_backup = std::chrono::duration_cast<std::chrono::hours>(
        now - last_backup_time_).count() / 24;
    
    return days_since_last_backup >= retention_days_;
}

bool BackupEngine::cleanupOldBackups() {
    try {
        // Implement cleanup logic based on retention policy
        // This could involve removing old backup files or snapshots
        logger_->log("Cleaning up old backups based on retention policy");
        return true;
    } catch (const std::exception& e) {
        logger_->log("Error during cleanup: " + std::string(e.what()), LogLevel::ERROR);
        return false;
    }
}

bool BackupEngine::cleanup() {
    try {
        if (nfs_handler_->isMounted()) {
            return nfs_handler_->unmount();
        }
        return true;
    } catch (const std::exception& e) {
        logger_->log("Error during cleanup: " + std::string(e.what()), LogLevel::ERROR);
        return false;
    }
} 