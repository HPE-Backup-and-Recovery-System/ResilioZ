#include "repositories/nfs_repository.h"

#include <filesystem>
#include <fstream>
#include <cstdlib>
#include <nlohmann/json.hpp>
#include <sys/mount.h>
#include <errno.h>
#include <string.h>
#include <sys/stat.h>

#include "utils/error_util.h"
#include "utils/prompter.h"
#include "utils/logger.h"

namespace fs = std::filesystem;

NFSRepository::NFSRepository() {}

NFSRepository::NFSRepository(const std::string& server_ip,
                            const std::string& server_backup_path,
                            const std::string& name,
                            const std::string& password,
                            const std::string& created_at) {
    server_ip_ = server_ip;
    server_backup_path_ = server_backup_path;
    name_ = name;
    password_ = password;
    created_at_ = created_at;
    type_ = RepositoryType::NFS;
    // Set the full path to the local mount point without double-nesting
    path_ = "/mnt/" + name;
}

bool NFSRepository::UploadFile(const std::string& local_file,
                               const std::string& remote_path) const {
  if (!fs::exists(local_file)) {
    ErrorUtil::ThrowError("Source file not found: " + local_file);
  }

  if (!fs::exists(remote_path)) {
    std::string mkdir_cmd = "sudo mkdir -p " + remote_path + 
                           " && sudo chmod u+w,go-w " + remote_path;
    int result = system(mkdir_cmd.c_str());
    if (result != 0) {
      ErrorUtil::ThrowError("Failed to create directory: " + remote_path);
    }
  }

  fs::path src_path(local_file);
  fs::path dest_path = fs::path(remote_path) / src_path.filename();
  
  std::string cp_cmd = "sudo cp " + local_file + " " + dest_path.string() + 
                      " && sudo chmod u+w,go-w " + dest_path.string();
  int result = system(cp_cmd.c_str());
  if (result != 0) {
    ErrorUtil::ThrowError("Failed to copy file to NFS share: " + dest_path.string());
  }

  return true;
}

bool NFSRepository::Exists() const {
    if (!NFSMountExists()) {
        return false;
    }

    // Check if config file exists
    std::string config_path = path_ + "/config.json";
    if (!fs::exists(config_path)) {
        Logger::Log("Config file not found at: " + config_path);
        return false;
    }

    // Try to read the config file
    try {
        std::ifstream file(config_path);
        if (!file.is_open()) {
            Logger::Log("Failed to open config file: " + config_path);
            return false;
        }
        nlohmann::json config;
        file >> config;
        return true;
    } catch (const std::exception& e) {
        Logger::Log("Error reading config file: " + std::string(e.what()));
        return false;
    }
}

void NFSRepository::Initialize() {
  EnsureNFSMounted();
  CreateRemoteDirectory();
  WriteConfig();
}

void NFSRepository::Delete() {
    try {
        // First check if it's mounted
        if (NFSMountExists()) {
            // Change permissions to allow unmounting
            std::string chmod_cmd = "sudo chmod -R 777 " + path_;
            int result = system(chmod_cmd.c_str());
            if (result != 0) {
                Logger::Log("Warning: Failed to set permissions for unmounting: " + path_);
            }

            // Unmount the NFS share
            std::string umount_cmd = "sudo umount -f " + path_;
            result = system(umount_cmd.c_str());
            if (result != 0) {
                Logger::Log("Warning: Failed to unmount NFS share: " + path_);
            }
        }

        // Remove the mount point directory
        std::string rm_cmd = "sudo rm -rf " + path_;
        int result = system(rm_cmd.c_str());
        if (result != 0) {
            Logger::Log("Warning: Failed to remove repository directory: " + path_);
        }

        Logger::Log("Successfully removed NFS repository: " + path_);
    } catch (const std::exception& e) {
        ErrorUtil::ThrowNested("Failed to remove NFS repository: " + path_);
    }
}

void NFSRepository::WriteConfig() const {
    // Ensure the directory exists before writing config
    std::string config_dir = path_;
    std::string mkdir_cmd = "sudo mkdir -p " + config_dir;
    int result = system(mkdir_cmd.c_str());
    if (result != 0) {
        ErrorUtil::ThrowError("Failed to create config directory: " + config_dir);
    }

    std::string config_path = config_dir + "/config.json";

    nlohmann::json config = {{"name", name_},
                           {"type", "nfs"},
                           {"path", path_},
                           {"server_ip", server_ip_},
                           {"server_backup_path", server_backup_path_},
                           {"created_at", created_at_},
                           {"password_hash", GetHashedPassword()}};

    // Create a temporary file first
    std::string temp_config_path = config_path + ".tmp";
    std::ofstream file(temp_config_path);
    if (!file.is_open()) {
        ErrorUtil::ThrowError("Failed to write config to: " + temp_config_path);
    }

    file << config.dump(4);
    file.close();

    // Move the temporary file to the final location
    std::string mv_cmd = "sudo mv " + temp_config_path + " " + config_path;
    result = system(mv_cmd.c_str());
    if (result != 0) {
        ErrorUtil::ThrowError("Failed to move config file to final location: " + config_path);
    }

    // Set proper permissions
    std::string chmod_cmd = "sudo chmod 644 " + config_path;
    result = system(chmod_cmd.c_str());
    if (result != 0) {
        Logger::Log("Warning: Failed to set permissions on config file: " + config_path);
    }

    // Verify the config file was written correctly
    if (!fs::exists(config_path)) {
        ErrorUtil::ThrowError("Config file was not created successfully: " + config_path);
    }
}

NFSRepository NFSRepository::FromConfigJson(const nlohmann::json& config) {
    try {
        return NFSRepository(config.at("server_ip"),
                           config.at("server_backup_path"),
                           config.at("name"),
                           config.value("password_hash", ""),
                           config.at("created_at"));
    } catch (const std::exception& e) {
        ErrorUtil::ThrowError("Invalid NFS config format: " + std::string(e.what()));
    }
}

bool NFSRepository::NFSMountExists() const { 
    // Check if the path exists and is a mount point
    struct stat st;
    if (stat(path_.c_str(), &st) != 0) {
        return false;
    }
    
    // Check if it's a mount point
    struct stat parent_st;
    if (stat((path_ + "/..").c_str(), &parent_st) != 0) {
        return false;
    }
    
    return st.st_dev != parent_st.st_dev;
}

void NFSRepository::EnsureNFSMounted() const {
    if (!NFSMountExists()) {
        // Create mount point directory first
        std::string mkdir_cmd = "sudo mkdir -p " + path_;
        int result = system(mkdir_cmd.c_str());
        if (result != 0) {
            ErrorUtil::ThrowError("Failed to create mount point directory: " + path_);
        }

        // First check if we have local data to preserve
        bool hasLocalData = false;
        std::string tempBackupPath = path_ + "_temp_backup";
        
        if (fs::exists(path_) && !fs::is_empty(path_)) {
            hasLocalData = true;
            // Create temporary backup of local data
            std::string backup_cmd = "sudo cp -r " + path_ + " " + tempBackupPath;
            result = system(backup_cmd.c_str());
            if (result != 0) {
                ErrorUtil::ThrowError("Failed to backup local data before mounting: " + path_);
            }
        }

        // Mount with specific options using server IP and backup path
        // Using sync option for better reliability
        std::string mount_cmd = "sudo mount -t nfs4 " + server_ip_ + ":" + server_backup_path_ + " " + path_;
        result = system(mount_cmd.c_str());
        if (result != 0) {
            // Try NFS3 if NFS4 fails
            mount_cmd = "sudo mount -t nfs " + server_ip_ + ":" + server_backup_path_ + " " + path_;
            result = system(mount_cmd.c_str());
            if (result != 0) {
                ErrorUtil::ThrowError("Failed to mount NFS share from " + server_ip_ + ":" + server_backup_path_ + 
                                    " to " + path_ + 
                                    "\nError: " + strerror(errno) +
                                    "\nPlease verify the NFS server is running and the export is available.");
            }
        }

        // If we had local data and the server directory is empty, restore it
        if (hasLocalData) {
            // Check if server directory is empty
            if (fs::is_empty(path_)) {
                // Restore local data to server
                std::string restore_cmd = "sudo cp -r " + tempBackupPath + "/* " + path_ + "/";
                result = system(restore_cmd.c_str());
                if (result != 0) {
                    ErrorUtil::ThrowError("Failed to restore local data to server: " + path_);
                }
                Logger::Log("Restored local data to server at: " + path_);
            }
            
            // Clean up temporary backup
            std::string cleanup_cmd = "sudo rm -rf " + tempBackupPath;
            system(cleanup_cmd.c_str());
        }
    }
}

void NFSRepository::CreateRemoteDirectory() const {
    // First ensure the mount point exists
    std::string cmd = "sudo mkdir -p " + path_;
    int result = system(cmd.c_str());
    if (result != 0) {
        ErrorUtil::ThrowError("Failed to create NFS repository directory: " + path_);
    }

    // Create a test file to verify permissions
    std::string test_file = path_ + "/.test_write";
    std::string touch_cmd = "touch " + test_file;
    result = system(touch_cmd.c_str());
    if (result != 0) {
        ErrorUtil::ThrowError("Failed to write to NFS share. Please check your permissions on the NFS server.");
    }

    // Clean up test file
    std::string rm_cmd = "rm -f " + test_file;
    system(rm_cmd.c_str());
}
