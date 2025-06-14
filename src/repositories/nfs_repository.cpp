#include "repositories/nfs_repository.h"

#include <filesystem>
#include <fstream>
#include <cstdlib>
#include <nlohmann/json.hpp>
#include <sys/mount.h>
#include <errno.h>
#include <string.h>
#include <sys/stat.h>
#include <thread>

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
    
    // Create base directory for NFS repositories if it doesn't exist
    std::string base_dir = "/mnt/nfs_repositories";
    std::string mkdir_cmd = "sudo mkdir -p " + base_dir;
    int result = system(mkdir_cmd.c_str());
    if (result != 0) {
        ErrorUtil::ThrowError("Failed to create NFS repositories base directory: " + base_dir);
    }
    
    // Set the path to the base directory
    path_ = base_dir;
}

bool NFSRepository::UploadFile(const std::string& local_file,
                               const std::string& remote_path) const {
  if (!fs::exists(local_file)) {
    ErrorUtil::ThrowError("Source file not found: " + local_file);
  }

  // Ensure the remote path exists
  std::string mkdir_cmd = "sudo mkdir -p " + remote_path;
    int result = system(mkdir_cmd.c_str());
    if (result != 0) {
      ErrorUtil::ThrowError("Failed to create directory: " + remote_path);
    }

  // Set proper permissions
  std::string chmod_cmd = "sudo chmod -R 777 " + remote_path;
  result = system(chmod_cmd.c_str());
  if (result != 0) {
    Logger::Log("Warning: Failed to set permissions on directory: " + remote_path);
  }

  // Copy the file directly to the remote path
  fs::path src_path(local_file);
  fs::path dest_path = fs::path(remote_path) / src_path.filename();
  
  std::string cp_cmd = "sudo cp " + local_file + " " + dest_path.string();
  result = system(cp_cmd.c_str());
  if (result != 0) {
    ErrorUtil::ThrowError("Failed to copy file to NFS share: " + dest_path.string());
  }

  // Set proper permissions on the copied file
  std::string file_chmod_cmd = "sudo chmod 666 " + dest_path.string();
  result = system(file_chmod_cmd.c_str());
  if (result != 0) {
    Logger::Log("Warning: Failed to set permissions on file: " + dest_path.string());
  }

  return true;
}

bool NFSRepository::Exists() const {
    if (!NFSMountExists()) {
        return false;
    }

    std::string config_path = path_ + "/" + name_ + "/config.json";
    if (!fs::exists(config_path)) {
        Logger::Log("Config file not found at: " + config_path);
        return false;
    }

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
        std::string repo_path = path_ + "/" + name_;
        
        if (NFSMountExists()) {
            // Delete files in backup directory using find without sudo
            std::string backup_path = repo_path + "/backup";
            std::string delete_backup_cmd = "find " + backup_path + " -type f -exec rm -f {} \\;";
            int result = system(delete_backup_cmd.c_str());
            if (result != 0) {
                Logger::Log("Warning: Failed to remove backup files: " + backup_path);
            }

            // Remove backup directories without sudo
            std::string rm_backup_dirs = "find " + backup_path + " -type d -exec rmdir {} \\; 2>/dev/null";
            result = system(rm_backup_dirs.c_str());
            if (result != 0) {
                Logger::Log("Warning: Failed to remove backup directories: " + backup_path);
            }

            // Delete files in chunks directory using find without sudo
            std::string chunks_path = repo_path + "/chunks";
            std::string delete_chunks_cmd = "find " + chunks_path + " -type f -exec rm -f {} \\;";
            result = system(delete_chunks_cmd.c_str());
            if (result != 0) {
                Logger::Log("Warning: Failed to remove chunk files: " + chunks_path);
            }

            // Remove chunk directories without sudo
            std::string rm_chunk_dirs = "find " + chunks_path + " -type d -exec rmdir {} \\; 2>/dev/null";
            result = system(rm_chunk_dirs.c_str());
            if (result != 0) {
                Logger::Log("Warning: Failed to remove chunk directories: " + chunks_path);
            }

            // Finally remove the repository directory itself
            // We might need sudo here since the parent directory is owned by nobody:nogroup
            std::string rm_cmd = "sudo rm -rf " + repo_path;
            result = system(rm_cmd.c_str());
            if (result != 0) {
                Logger::Log("Warning: Failed to remove repository directory: " + repo_path);
            }

            // Verify if anything remains
            if (fs::exists(repo_path)) {
                Logger::Log("Warning: Some files could not be deleted in: " + repo_path);
            } else {
                Logger::Log("Successfully removed NFS repository: " + repo_path);
            }
        } else {
            Logger::Log("NFS mount not found, skipping repository deletion");
        }
    } catch (const std::exception& e) {
        ErrorUtil::ThrowNested("Failed to remove NFS repository: " + name_);
    }
}

void NFSRepository::WriteConfig() const {
    // First ensure the repository directory exists on the NFS share
    std::string repo_path = path_ + "/" + name_;
    std::string mkdir_cmd = "sudo mkdir -p " + repo_path;
    int result = system(mkdir_cmd.c_str());
    if (result != 0) {
        ErrorUtil::ThrowError("Failed to create repository directory on NFS share: " + repo_path);
    }

    std::string config_path = repo_path + "/config.json";
    std::string temp_config_path = "/tmp/config.json.tmp";

    nlohmann::json config = {{"name", name_},
                           {"type", "nfs"},
                           {"path", path_},
                           {"server_ip", server_ip_},
                           {"server_backup_path", server_backup_path_},
                           {"created_at", created_at_},
                           {"password_hash", GetHashedPassword()}};

    // Write to temporary file first
    std::ofstream file(temp_config_path);
    if (!file.is_open()) {
        ErrorUtil::ThrowError("Failed to write config to temporary file: " + temp_config_path);
    }

    file << config.dump(4);
    file.close();

    // Move the temporary file to the final location with sudo
    std::string mv_cmd = "sudo mv " + temp_config_path + " " + config_path;
    result = system(mv_cmd.c_str());
    if (result != 0) {
        ErrorUtil::ThrowError("Failed to move config file to final location: " + config_path);
    }

    // Set proper ownership and permissions
    std::string chown_cmd = "sudo chown $USER:$USER " + config_path;
    result = system(chown_cmd.c_str());
    if (result != 0) {
        Logger::Log("Warning: Failed to set ownership on config file: " + config_path);
    }

    std::string chmod_cmd = "sudo chmod 644 " + config_path;
    result = system(chmod_cmd.c_str());
    if (result != 0) {
        Logger::Log("Warning: Failed to set permissions on config file: " + config_path);
    }

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
    struct stat st;
    if (stat("/mnt/nfs_repositories", &st) != 0) {
        return false;
    }
    
    struct stat parent_st;
    if (stat("/mnt", &parent_st) != 0) {
        return false;
    }
    
    return st.st_dev != parent_st.st_dev;
}

void NFSRepository::EnsureNFSMounted() const {
    // Check if the base directory exists
    struct stat st;
    if (stat("/mnt/nfs_repositories", &st) != 0) {
        std::string mkdir_cmd = "sudo mkdir -p /mnt/nfs_repositories";
        int result = system(mkdir_cmd.c_str());
        if (result != 0) {
            ErrorUtil::ThrowError("Failed to create NFS mount point: /mnt/nfs_repositories");
        }
    }
    
    struct stat parent_st;
    if (stat("/mnt", &parent_st) != 0) {
        ErrorUtil::ThrowError("Parent directory does not exist");
    }
    
    // If not mounted, mount the entire NFS share
    if (st.st_dev == parent_st.st_dev) {
        std::string umount_cmd = "sudo umount -f /mnt/nfs_repositories 2>/dev/null";
        system(umount_cmd.c_str()); // Ignore errors from unmount

        int max_retries = 3;
        int retry_count = 0;
        bool mount_success = false;
        bool server_reachable = false;

        while (retry_count < max_retries && !mount_success) {
            if (retry_count > 0) {
                Logger::Log("Re-establishing connection to NFS server...");
                int wait_time = 5 * (1 << retry_count); // Start with 5 seconds, then 10, then 20
                std::this_thread::sleep_for(std::chrono::seconds(wait_time));
            }

            // First check if server responds to ping
            std::string ping_cmd = "ping -c 1 -W 5 " + server_ip_ + " > /dev/null 2>&1";
            int result = system(ping_cmd.c_str());
            
            if (result == 0) {
                server_reachable = true;
                Logger::Log("Server " + server_ip_ + " is reachable, attempting to mount...");

                // Try simple mount first
                std::string mount_cmd = "sudo mount -t nfs " + server_ip_ + ":" + server_backup_path_ + " /mnt/nfs_repositories";
                result = system(mount_cmd.c_str());
                
                if (result == 0) {
                    mount_success = true;
                    Logger::Log("Successfully mounted NFS share");
                    break;
                }

                // If simple mount fails, try with explicit options
                mount_cmd = "sudo mount -t nfs -o rw,soft,intr " + server_ip_ + ":" + server_backup_path_ + " /mnt/nfs_repositories";
        result = system(mount_cmd.c_str());
                
                if (result == 0) {
                    mount_success = true;
                    Logger::Log("Successfully mounted NFS share with explicit options");
                    break;
                }

                // Log the specific mount error
                Logger::Log("Mount attempt failed: " + std::string(strerror(errno)), LogLevel::WARNING);

                if (retry_count < max_retries - 1) {
                    Logger::Log("Mount failed but server is reachable. Will retry in " + std::to_string(5 * (1 << (retry_count + 1))) + " seconds...");
                }
            } else {
                Logger::Log("Server " + server_ip_ + " is not responding to ping", LogLevel::WARNING);
            }

            retry_count++;
        }

        if (!mount_success) {
            std::string error_msg = "Failed to mount NFS share from " + server_ip_ + ":" + server_backup_path_ + 
                                  " to /mnt/nfs_repositories\n";
            
            if (!server_reachable) {
                error_msg += "Server is not reachable. Please check:\n";
                error_msg += "1. The server IP address is correct\n";
                error_msg += "2. The server is running\n";
                error_msg += "3. There are no firewall rules blocking access\n";
            } else {
                error_msg += "Server is reachable but mount failed. Please check:\n";
                error_msg += "1. The NFS service is running on the server\n";
                error_msg += "2. The export path '" + server_backup_path_ + "' exists and is exported\n";
                error_msg += "3. The NFS export permissions allow this client to mount\n";
                error_msg += "4. Error details: " + std::string(strerror(errno)) + "\n";
            }
            
            ErrorUtil::ThrowError(error_msg);
        }

        // Set proper permissions on the mount point
        std::string chmod_cmd = "sudo chmod 777 /mnt/nfs_repositories";
        int chmod_result = system(chmod_cmd.c_str());
        if (chmod_result != 0) {
            Logger::Log("Warning: Failed to set permissions on NFS mount point");
        }
    }

    // Create the repository directory if it doesn't exist
    std::string repo_path = path_ + "/" + name_;
    std::string mkdir_cmd = "sudo mkdir -p " + repo_path;
    int mkdir_result = system(mkdir_cmd.c_str());
    if (mkdir_result != 0) {
        ErrorUtil::ThrowError("Failed to create repository directory: " + repo_path);
    }
            
    // Set proper permissions on the repository directory
    std::string chmod_cmd = "sudo chmod 777 " + repo_path;
    int chmod_result = system(chmod_cmd.c_str());
    if (chmod_result != 0) {
        Logger::Log("Warning: Failed to set permissions on repository directory: " + repo_path);
    }
}

void NFSRepository::CreateRemoteDirectory() const {
    std::string cmd = "sudo mkdir -p " + path_;
    int result = system(cmd.c_str());
    if (result != 0) {
        ErrorUtil::ThrowError("Failed to create NFS repository directory: " + path_);
    }

    std::string test_file = path_ + "/.test_write";
    std::string touch_cmd = "touch " + test_file;
    result = system(touch_cmd.c_str());
    if (result != 0) {
        ErrorUtil::ThrowError("Failed to write to NFS share. Please check your permissions on the NFS server.");
    }

    std::string rm_cmd = "rm -f " + test_file;
    result = system(rm_cmd.c_str());
    if (result != 0) {
        Logger::Log("Warning: Failed to remove test file: " + test_file);
    }
}
