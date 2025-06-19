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
    ErrorUtil::ThrowError("Failed to copy file to NFS share: " +
                          dest_path.string());
  }

  return true;
}

bool NFSRepository::Exists() const {
  if (!NFSMountExists()) {
    return false;
  }

  std::string config_path = path_ + "/config.json";
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
    if (NFSMountExists()) {
      std::string chmod_cmd = "sudo chmod -R 777 " + path_;
      int result = system(chmod_cmd.c_str());
      if (result != 0) {
        Logger::Log("Warning: Failed to set permissions for unmounting: " +
                    path_);
      }

      std::string umount_cmd = "sudo umount -f " + path_;
      result = system(umount_cmd.c_str());
      if (result != 0) {
        Logger::Log("Warning: Failed to unmount NFS share: " + path_);
      }
    }

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

  std::string temp_config_path = config_path + ".tmp";
  std::ofstream file(temp_config_path);
  if (!file.is_open()) {
    ErrorUtil::ThrowError("Failed to write config to: " + temp_config_path);
  }

  file << config.dump(4);
  file.close();

  std::string mv_cmd = "sudo mv " + temp_config_path + " " + config_path;
  result = system(mv_cmd.c_str());
  if (result != 0) {
    ErrorUtil::ThrowError("Failed to move config file to final location: " +
                          config_path);
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
  if (stat(path_.c_str(), &st) != 0) {
    return false;
  }

  struct stat parent_st;
  if (stat((path_ + "/..").c_str(), &parent_st) != 0) {
    return false;
  }

  return st.st_dev != parent_st.st_dev;
}

void NFSRepository::EnsureNFSMounted() const {
  if (!NFSMountExists()) {
    std::string mkdir_cmd = "sudo mkdir -p " + path_;
    int result = system(mkdir_cmd.c_str());
    if (result != 0) {
      ErrorUtil::ThrowError("Failed to create mount point directory: " + path_);
    }

    bool hasLocalData = false;
    std::string tempBackupPath = path_ + "_temp_backup";

    if (fs::exists(path_) && !fs::is_empty(path_)) {
      hasLocalData = true;
      std::string backup_cmd = "sudo cp -r " + path_ + " " + tempBackupPath;
      result = system(backup_cmd.c_str());
      if (result != 0) {
        ErrorUtil::ThrowError("Failed to backup local data before mounting: " +
                              path_);
      }
    }

    int max_retries = 3;
    int retry_count = 0;
    bool mount_success = false;
    bool server_reachable = false;

    while (retry_count < max_retries && !mount_success) {
      if (retry_count > 0) {
        Logger::Log("Re-establishing connection to NFS server...");
        int wait_time = 20 * (1 << retry_count);
        std::this_thread::sleep_for(std::chrono::seconds(wait_time));
      }

      std::string ping_cmd =
          "ping -c 1 -W 5 " + server_ip_ + " > /dev/null 2>&1";
      result = system(ping_cmd.c_str());

      if (result == 0) {
        server_reachable = true;
        std::string mount_cmd = "sudo mount -t nfs4 " + server_ip_ + ":" +
                                server_backup_path_ + " " + path_;
        result = system(mount_cmd.c_str());

        if (result == 0) {
          mount_success = true;
          break;
        }

        mount_cmd = "sudo mount -t nfs " + server_ip_ + ":" +
                    server_backup_path_ + " " + path_;
        result = system(mount_cmd.c_str());

        if (result == 0) {
          mount_success = true;
          break;
        }

        if (retry_count < max_retries - 1) {
          Logger::Log(
              "Mount failed but server is reachable. Re-attempting mount...");
        }
      }

      retry_count++;
    }

    if (!mount_success) {
      std::string error_msg = "Failed to mount NFS share from " + server_ip_ +
                              ":" + server_backup_path_ + " to " + path_ +
                              "\nError: " + strerror(errno) +
                              "\nPlease verify the NFS server is running and "
                              "the export is available.";

      if (!server_reachable) {
        error_msg =
            "Unable to establish connection with the server\n" + error_msg;
      }

      ErrorUtil::ThrowError(error_msg);
    }

    if (hasLocalData) {
      if (fs::is_empty(path_)) {
        std::string restore_cmd =
            "sudo cp -r " + tempBackupPath + "/* " + path_ + "/";
        result = system(restore_cmd.c_str());
        if (result != 0) {
          ErrorUtil::ThrowError("Failed to restore local data to server: " +
                                path_);
        }
        Logger::Log("Restored local data to server at: " + path_);
      }

      std::string cleanup_cmd = "sudo rm -rf " + tempBackupPath;
      system(cleanup_cmd.c_str());
    }
  }
}

void NFSRepository::CreateRemoteDirectory() const {
  std::string cmd = "sudo mkdir -p " + path_;
  int result = system(cmd.c_str());
  if (result != 0) {
    ErrorUtil::ThrowError("Failed to create NFS repository directory: " +
                          path_);
  }

  std::string test_file = path_ + "/.test_write";
  std::string touch_cmd = "touch " + test_file;
  result = system(touch_cmd.c_str());
  if (result != 0) {
    ErrorUtil::ThrowError(
        "Failed to write to NFS share. Please check your permissions on the "
        "NFS server.");
  }

  std::string rm_cmd = "rm -f " + test_file;
  system(rm_cmd.c_str());
}
