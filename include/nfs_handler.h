#pragma once

#include <string>
#include <memory>
#include <nlohmann/json.hpp>
#include "logger.h"

using json = nlohmann::json;

class NFSHandler {
private:
    std::shared_ptr<Logger> logger_;
    std::string nfs_server_;
    std::string mount_point_;
    bool is_mounted_;

    bool mountNFS();
    bool unmountNFS();
    bool checkMountStatus();
    
public:
    NFSHandler(const std::string& config_path, std::shared_ptr<Logger> logger);
    ~NFSHandler();

    // Core NFS operations
    bool initialize();
    bool mountNFSShare();
    bool unmount();
    bool isMounted() const { return is_mounted_; }
    
    // Backup operations
    bool performBackup(const std::string& source_path);
    bool verifyBackup(const std::string& source_path);
    
    // Getters
    std::string getNFSServer() const { return nfs_server_; }
    std::string getMountPoint() const { return mount_point_; }
};