#include "repositories/nfs_repository.h"

#include <filesystem>
#include <fstream>
#include <iostream>
#include <nlohmann/json.hpp>

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
}

std::string NFSRepository::GetNFSPath() const { return path_ + "/" + name_; }

bool NFSRepository::UploadFile(const std::string& local_file,
                               const std::string& remote_path) const {
  try {
    if (!fs::exists(local_file)) {
      std::cerr << "Source File Not Found: " << local_file << "\n";
      return false;
    }

    if (!fs::exists(remote_path)) {
      fs::create_directories(remote_path);
    }

    fs::path src_path(local_file);
    fs::path dest_path = fs::path(remote_path) / src_path.filename();

    fs::copy_file(src_path, dest_path, fs::copy_options::overwrite_existing);
    return true;

  } catch (const fs::filesystem_error& e) {
    std::cerr << "ERROR in NFS FileSystem: " << e.what() << "\n";
    return false;

  } catch (const std::exception& e) {
    std::cerr << "GENERAL ERROR: " << e.what() << "\n";
    return false;
  }
}

bool NFSRepository::Exists() const {
  try {
    return NFSMountExists() && fs::exists(GetNFSPath());
  } catch (const fs::filesystem_error& e) {
    std::cerr << "ERROR Checking NFS Existence: " << e.what() << "\n";
    return false;
  }
}

void NFSRepository::Initialize() {
  try {
    EnsureNFSMounted();
    CreateRemoteDirectory(); 
    WriteConfig();
  } catch (const std::exception& e) {
    std::cerr << "ERROR Initializing NFS Repo: " << e.what() << "\n";
    throw;
  }
}

void NFSRepository::Delete() {
  try {
    RemoveRemoteDirectory();
  } catch (const fs::filesystem_error& e) {
    std::cerr << "ERROR Deleting NFS Repo: " << e.what() << "\n";
    throw;
  }
}

void NFSRepository::WriteConfig() const {
  std::string config_path = GetNFSPath() + "/config.json";
  try {
    nlohmann::json config = {{"name", name_},
                             {"type", "nfs"},
                             {"path", path_},
                             {"created_at", created_at_},
                             {"password_hash", GetHashedPassword()}};

    std::ofstream file(config_path);
    if (file.is_open()) {
      file << config.dump(4);
      file.close();
    } else {
      std::cerr << "Failed to Write NFS Config to: " << config_path << "\n";
    }
  } catch (const std::exception& e) {
    std::cerr << "ERROR Writing NFS Config: " << e.what() << "\n";
  }
}

NFSRepository NFSRepository::FromConfigJson(const nlohmann::json& config) {
  try {
    return NFSRepository(config.at("path"), config.at("name"),
                         config.value("password_hash", ""),
                         config.at("created_at"));
  } catch (const std::exception& e) {
    std::cerr << "ERROR Parsing NFS Config JSON: " << e.what() << "\n";
    return NFSRepository();
  }
}

bool NFSRepository::NFSMountExists() const {
  return fs::exists(path_);  
}

void NFSRepository::EnsureNFSMounted() const {
  if (!NFSMountExists()) {
    throw std::runtime_error("NFS Mount Path Not Available: " + path_);
  }
}

void NFSRepository::CreateRemoteDirectory() const {
  try {
    fs::create_directories(GetNFSPath());
  } catch (const fs::filesystem_error& e) {
    std::cerr << "ERROR Creating NFS Directory: " << e.what() << "\n";
    throw;
  }
}

void NFSRepository::RemoveRemoteDirectory() const {
  try {
    fs::remove_all(GetNFSPath());
  } catch (const fs::filesystem_error& e) {
    std::cerr << "ERROR Removing NFS Directory: " << e.what() << "\n";
    throw;
  }
}