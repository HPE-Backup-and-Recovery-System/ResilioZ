#include "nfs_handler.h"
#include <fstream>
#include <filesystem>
#include <sys/mount.h>
#include <sys/stat.h>
#include <unistd.h>
#include <cstring>

namespace fs = std::filesystem;

NFSHandler::NFSHandler(const std::string& config_path, std::shared_ptr<Logger> logger)
    : logger_(logger), is_mounted_(false) {
    try {
        std::ifstream f(config_path);
        json config = json::parse(f);
        nfs_server_ = config["nfs_server"].get<std::string>();
        mount_point_ = config["mount_point"].get<std::string>();
    } catch (const std::exception& e) {
        logger_->log("Error loading config: " + std::string(e.what()), LogLevel::ERROR);
        throw;
    }
}

NFSHandler::~NFSHandler() {
    if (is_mounted_) {
        unmount();
    }
}

bool NFSHandler::initialize() {
    // Create mount point if it doesn't exist
    if (!fs::exists(mount_point_)) {
        try {
            fs::create_directories(mount_point_);
            logger_->log("Created mount point: " + mount_point_);
        } catch (const std::exception& e) {
            logger_->log("Failed to create mount point: " + std::string(e.what()), LogLevel::ERROR);
            return false;
        }
    }
    return true;
}

bool NFSHandler::mountNFSShare() {
    if (is_mounted_) {
        logger_->log("NFS already mounted", LogLevel::WARNING);
        return true;
    }

    if (!initialize()) {
        return false;
    }

    return mountNFS();
}

bool NFSHandler::mountNFS() {
    // First try to unmount in case of stale mount
    if (fs::exists(mount_point_)) {
        std::string unmount_cmd = "umount " + mount_point_;
        system(unmount_cmd.c_str());
    }

    // Use the simplest mount command, matching the successful manual mount
    std::string mount_cmd = "mount -t nfs " + nfs_server_ + " " + mount_point_;
    int mount_result = system(mount_cmd.c_str());

    if (mount_result != 0) {
        logger_->log("Failed to mount NFS: " + std::string(strerror(errno)), LogLevel::ERROR);
        return false;
    }

    is_mounted_ = true;
    logger_->log("Successfully mounted NFS at: " + mount_point_);
    return true;
}
    
bool NFSHandler::unmount() {
    if (!is_mounted_) {
        logger_->log("NFS not mounted", LogLevel::WARNING);
        return true;
    }

    return unmountNFS();
}

bool NFSHandler::unmountNFS() {
    int unmount_result = umount(mount_point_.c_str());
    
    if (unmount_result != 0) {
        logger_->log("Failed to unmount NFS: " + std::string(strerror(errno)), LogLevel::ERROR);
        return false;
    }

    is_mounted_ = false;
    logger_->log("Successfully unmounted NFS from: " + mount_point_);
    return true;
}

bool NFSHandler::performBackup(const std::string& source_path) {
    if (!is_mounted_) {
        logger_->log("Cannot perform backup: NFS not mounted", LogLevel::ERROR);
        return false;
    }

    try {
        // Create backup command using rsync with options to ignore permissions and timestamps
        std::string cmd = "rsync -avz --delete --no-owner --no-group --no-times --no-perms " + 
                         source_path + "/ " + mount_point_ + "/ 2>&1";
        
        // Use popen to capture both stdout and stderr
        FILE* pipe = popen(cmd.c_str(), "r");
        if (!pipe) {
            logger_->log("Failed to execute rsync command", LogLevel::ERROR);
            return false;
        }

        // Read the output
        char buffer[256];
        std::string result;
        while (fgets(buffer, sizeof(buffer), pipe) != NULL) {
            result += buffer;
    }
    
        // Close pipe and get status
        int status = pclose(pipe);
        
        if (status != 0) {
            logger_->log("Backup failed with output: " + result, LogLevel::ERROR);
            return false;
        }

        logger_->log("Backup completed successfully");
        return true;
    } catch (const std::exception& e) {
        logger_->log("Backup failed: " + std::string(e.what()), LogLevel::ERROR);
        return false;
    }
}

bool NFSHandler::verifyBackup(const std::string& source_path) {
    if (!is_mounted_) {
        logger_->log("Cannot verify backup: NFS not mounted", LogLevel::ERROR);
        return false;
    }

    try {
        // Use rsync dry-run to verify, with options to ignore permissions and timestamps
        std::string cmd = "rsync -avzn --delete --no-owner --no-group --no-times --no-perms " + 
                         source_path + "/ " + mount_point_ + "/ 2>&1";
        
        // Use popen to capture both stdout and stderr
        FILE* pipe = popen(cmd.c_str(), "r");
        if (!pipe) {
            logger_->log("Failed to execute rsync command", LogLevel::ERROR);
            return false;
        }

        // Read the output
        char buffer[256];
        std::string result;
        while (fgets(buffer, sizeof(buffer), pipe) != NULL) {
            result += buffer;
        }

        // Close pipe and get status
        int status = pclose(pipe);
        
        if (status != 0) {
            logger_->log("Backup verification failed with output: " + result, LogLevel::ERROR);
            return false;
    }

        logger_->log("Backup verification completed successfully");
        return true;
    } catch (const std::exception& e) {
        logger_->log("Backup verification failed: " + std::string(e.what()), LogLevel::ERROR);
        return false;
    }
}