#include "repositories/nfs_repository.h"

#include <errno.h>
#include <string.h>
#include <sys/mount.h>
#include <sys/stat.h>

#include <cstdlib>
#include <filesystem>
#include <fstream>
#include <nlohmann/json.hpp>
#include <thread>

#include "utils/error_util.h"
#include "utils/logger.h"
#include "utils/prompter.h"

namespace fs = std::filesystem;

NFSRepository::NFSRepository() {}

NFSRepository::NFSRepository(const std::string& nfs_mount_path,
                             const std::string& name,
                             const std::string& password,
                             const std::string& created_at) {
  path_ = nfs_mount_path;
  name_ = name;
  password_ = password;
  created_at_ = created_at;
  type_ = RepositoryType::NFS;
}

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
    // Mount point will be set via SetMountPoint
}

void NFSRepository::SetMountPoint(const std::string& mount_point) {
    mount_point_ = mount_point;
    path_ = mount_point_;
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
  std::string chmod_cmd = "sudo chmod -R 777 " + remote_path; // -R 777 is unsafe, please check 
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
        std::string repo_path_str = path_ + "/" + name_;
        fs::path repo_path(repo_path_str);
        
        if (NFSMountExists()) {
            if (fs::exists(repo_path)) {
                try {
                    fs::remove_all(repo_path);
                } catch (const fs::filesystem_error& e) {
                    ErrorUtil::ThrowError("Failed to remove repository '" + name_ + "': " + e.what() + 
                                          ".\nThis is likely a file permission issue. Files on the NFS share seem to be created with 'sudo', which may prevent this program from deleting them."
                                          " Consider running this program with 'sudo' or fixing file permissions on the share.");
                }
            }

            if (fs::exists(repo_path)) {
                Logger::Log("Warning: Repository directory could not be fully deleted: " + repo_path_str, LogLevel::WARNING);
            } else {
                Logger::Log("Successfully removed NFS repository: " + repo_path_str);
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
    Logger::Log("Warning: Failed to set permissions on config file: " +
                config_path);
  }

  if (!fs::exists(config_path)) {
    ErrorUtil::ThrowError("Config file was not created successfully: " +
                          config_path);
  }
}

NFSRepository NFSRepository::FromConfigJson(const nlohmann::json& config) {
  try {
    return NFSRepository(config.at("server_ip"),
                         config.at("server_backup_path"), config.at("name"),
                         config.value("password_hash", ""),
                         config.at("created_at"));
  } catch (const std::exception& e) {
    ErrorUtil::ThrowError("Invalid NFS config format: " +
                          std::string(e.what()));
  }
}

bool NFSRepository::NFSMountExists() const { 
    struct stat st;
    if (stat(mount_point_.c_str(), &st) != 0) {
        return false;
    }
    
    struct stat parent_st;
    std::string parent_dir = fs::path(mount_point_).parent_path().string();
    if (stat(parent_dir.c_str(), &parent_st) != 0) {
        return false;
    }
    
    return st.st_dev != parent_st.st_dev;
}

bool NFSRepository::MountNFSShare() const {
    // Create mount point if it doesn't exist
    if (!fs::exists(mount_point_)) {
        std::string mkdir_cmd = "sudo mkdir -p " + mount_point_;
        int result = system(mkdir_cmd.c_str());
        if (result != 0) {
            Logger::Log("Failed to create NFS mount point: " + mount_point_, LogLevel::ERROR);
            return false;
        }
        Logger::Log("Created mount point: " + mount_point_);
    }

    // Check if already mounted
    bool currently_mounted = false;
    try {
        struct stat st;
        struct stat parent_st;
        std::string parent_dir = fs::path(mount_point_).parent_path().string();
        
        if (stat(mount_point_.c_str(), &st) == 0 && 
            stat(parent_dir.c_str(), &parent_st) == 0) {
            currently_mounted = (st.st_dev != parent_st.st_dev);
        }
    } catch (const std::exception& e) {
        Logger::Log("Warning: Error checking mount status: " + std::string(e.what()), LogLevel::WARNING);
    }

    // If mounted, unmount first
    if (currently_mounted) {
        Logger::Log("Unmounting existing NFS mount...");
        std::string umount_cmd = "sudo umount -f " + mount_point_ + " 2>/dev/null";
        int umount_result = system(umount_cmd.c_str());
        if (umount_result != 0) {
            Logger::Log("Warning: Failed to unmount existing mount point", LogLevel::WARNING);
        }
    }

    int max_retries = 3;
    int retry_count = 0;
    bool mount_success = false;
    std::string mount_output;

    while (retry_count < max_retries && !mount_success) {
        if (retry_count > 0) {
            Logger::Log("Re-establishing connection to NFS server...");
            int wait_time = 5 * (1 << retry_count); // Start with 5 seconds, then 10, then 20
            std::this_thread::sleep_for(std::chrono::seconds(wait_time));
        }

        Logger::Log("Attempting to mount NFS share from " + server_ip_ + ":" + server_backup_path_);

        // Try simple mount first
        std::string mount_cmd = "sudo mount -t nfs " + server_ip_ + ":" + server_backup_path_ + " " + mount_point_ + " 2>&1";
        FILE* pipe = popen(mount_cmd.c_str(), "r");
        if (pipe) {
            char buffer[128];
            while (fgets(buffer, sizeof(buffer), pipe) != nullptr) {
                mount_output += buffer;
            }
            int result = pclose(pipe);
            if (result == 0) {
                mount_success = true;
                Logger::Log("Successfully mounted NFS share");
                break;
            }
            Logger::Log("Mount attempt failed: " + mount_output, LogLevel::WARNING);
        }

        // If simple mount fails, try with explicit options
        if (!mount_success) {
            mount_cmd = "sudo mount -t nfs -o rw,soft,intr " + server_ip_ + ":" + server_backup_path_ + " " + mount_point_ + " 2>&1";
            pipe = popen(mount_cmd.c_str(), "r");
            if (pipe) {
                char buffer[128];
                mount_output.clear();
                while (fgets(buffer, sizeof(buffer), pipe) != nullptr) {
                    mount_output += buffer;
                }
                int result = pclose(pipe);
                if (result == 0) {
                    mount_success = true;
                    Logger::Log("Successfully mounted NFS share with explicit options");
                    break;
                }
                Logger::Log("Mount attempt with options failed: " + mount_output, LogLevel::WARNING);
            }
        }

        retry_count++;
    }

    if (mount_success) {
        // Set proper permissions on the mount point
        std::string chown_cmd = "sudo chown $USER:$USER " + mount_point_;
        int chown_result = system(chown_cmd.c_str());
        if (chown_result != 0) {
            Logger::Log("Warning: Failed to set ownership on mount point", LogLevel::WARNING);
        }

        std::string chmod_cmd = "sudo chmod 777 " + mount_point_;
        int chmod_result = system(chmod_cmd.c_str());
        if (chmod_result != 0) {
            Logger::Log("Warning: Failed to set permissions on mount point", LogLevel::WARNING);
        }

        // Create the repository directory if it doesn't exist
        std::string repo_path = path_ + "/" + name_;
        std::string mkdir_cmd = "sudo mkdir -p " + repo_path;
        int mkdir_result = system(mkdir_cmd.c_str());
        if (mkdir_result != 0) {
            Logger::Log("Warning: Failed to create repository directory", LogLevel::WARNING);
        }
                
        // Set proper permissions on the repository directory
        chmod_cmd = "sudo chmod 777 " + repo_path;
        chmod_result = system(chmod_cmd.c_str());
        if (chmod_result != 0) {
            Logger::Log("Warning: Failed to set permissions on repository directory", LogLevel::WARNING);
        }

        chown_cmd = "sudo chown $USER:$USER " + repo_path;
        chown_result = system(chown_cmd.c_str());
        if (chown_result != 0) {
            Logger::Log("Warning: Failed to set ownership on repository directory", LogLevel::WARNING);
        }
    }

    return mount_success;
}

void NFSRepository::EnsureNFSMounted() const {
    // First check if server responds to ping
    Logger::Log("Checking if server " + server_ip_ + " is reachable...");
    std::string ping_cmd = "ping -c 1 -W 5 " + server_ip_ + " > /dev/null 2>&1";
    int result = system(ping_cmd.c_str());
    bool server_reachable = (result == 0);
    
    if (!server_reachable) {
        std::string error_msg = "Server " + server_ip_ + " is not reachable. Please check:\n";
        error_msg += "1. The server IP address is correct\n";
        error_msg += "2. The server is running\n";
        error_msg += "3. There are no firewall rules blocking access\n";
        error_msg += "4. The server is actually an NFS server (not just any pingable machine)\n";
        ErrorUtil::ThrowError(error_msg);
    }

    Logger::Log("Server " + server_ip_ + " is reachable, attempting to mount...");

    if (!MountNFSShare()) {
        std::string error_msg = "Failed to mount NFS share from " + server_ip_ + ":" + server_backup_path_ + 
                              " to " + mount_point_ + "\n";
        error_msg += "Server is reachable but mount failed. Please check:\n";
        error_msg += "1. The NFS service is running on the server\n";
        error_msg += "2. The export path '" + server_backup_path_ + "' exists and is exported\n";
        error_msg += "3. The NFS export permissions allow this client to mount\n";
        error_msg += "4. The server is actually running an NFS service\n";
        ErrorUtil::ThrowError(error_msg);
    }
}

void NFSRepository::CreateRemoteDirectory() const {
    // No need to create the path directory since it's already created in EnsureNFSMounted
    
    // Test write access with sudo
    std::string test_file = path_ + "/.test_write";
    std::string touch_cmd = "sudo touch " + test_file;
    int result = system(touch_cmd.c_str());
    if (result != 0) {
        ErrorUtil::ThrowError("Failed to write to NFS share. Please check your permissions on the NFS server.");
    }

    // Set permissions on the test file
    std::string chmod_cmd = "sudo chmod 666 " + test_file;
    result = system(chmod_cmd.c_str());
    if (result != 0) {
        Logger::Log("Warning: Failed to set permissions on test file");
    }

    // Try to remove the test file without sudo (to verify user permissions)
    std::string rm_cmd = "rm -f " + test_file;
    result = system(rm_cmd.c_str());
    if (result != 0) {
        // If regular remove fails, use sudo
        rm_cmd = "sudo rm -f " + test_file;
        result = system(rm_cmd.c_str());
        if (result != 0) {
            Logger::Log("Warning: Failed to remove test file: " + test_file);
        }
    }
}
