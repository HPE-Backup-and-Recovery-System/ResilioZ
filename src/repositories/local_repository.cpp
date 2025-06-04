#include "repositories/local_repository.h"

#include <filesystem>
#include <fstream>
#include <iostream>
#include <nlohmann/json.hpp>

namespace fs = std::filesystem;

LocalRepository::LocalRepository() {}

LocalRepository::LocalRepository(const std::string& path,
                                 const std::string& name,
                                 const std::string& password,
                                 const std::string& created_at) {
  path_ = path, name_ = name, password_ = password, created_at_ = created_at;
}

std::string LocalRepository::GetFullPath() const { return path_ + "/" + name_; }

bool LocalRepository::UploadFile(const std::string& local_file,
                                 const std::string& local_path) const {
  try {
    if (!fs::exists(local_file)) {
      std::cerr << "Source File Not Found at: " << local_file << "\n";
      return false;
    }

    if (!fs::exists(local_path)) {
      fs::create_directories(local_path);
    }
    fs::path src_path(local_file);
    fs::path dest_path = fs::path(local_path) / src_path.filename();

    fs::copy_file(src_path, dest_path, fs::copy_options::overwrite_existing);
    return true;

  } catch (const fs::filesystem_error& e) {
    std::cerr << "ERROR in FileSystem: " << e.what() << "\n";
    return false;

  } catch (const std::exception& e) {
    std::cerr << "ERROR: " << e.what() << "\n";
    return false;
  }
}

bool LocalRepository::Exists() const {
  try {
    return LocalDirectoryExists();
  } catch (const fs::filesystem_error& e) {
    std::cerr << "ERROR Checking Existence: " << e.what() << "\n";
    return false;
  }
}

void LocalRepository::Initialize() {
  try {
    CreateLocalDirectory();
    WriteConfig();
  } catch (const std::exception& e) {
    std::cerr << "ERROR Initializing Repository: " << e.what() << "\n";
    throw;
  }
}

void LocalRepository::Delete() {
  try {
    RemoveLocalDirectory();
  } catch (const fs::filesystem_error& e) {
    std::cerr << "ERROR Deleting Repository: " << e.what() << "\n";
  }
}

void LocalRepository::WriteConfig() const {
  std::string config_path = GetFullPath() + "/config.json";
  try {
    nlohmann::json config = {{"name", name_},
                             {"type", "local"},
                             {"path", path_},
                             {"created_at", created_at_},
                             {"password_hash", GetHashedPassword()}};

    std::ofstream file(config_path);
    if (file.is_open()) {
      file << config.dump(4);
      file.close();
    } else {
      std::cerr << "Failed to Write Config to: " << config_path << "\n";
    }
  } catch (const std::exception& e) {
    std::cerr << "ERROR Writing Config File: " << e.what() << "\n";
    throw;
  }
}

LocalRepository LocalRepository::FromConfigJson(const nlohmann::json& config) {
  try {
    return LocalRepository(config.at("path"), config.at("name"),
                           config.value("password_hash", ""),
                           config.at("created_at"));
  } catch (const std::exception& e) {
    std::cerr << "ERROR Parsing Config JSON: " << e.what() << "\n";
    return LocalRepository();
  }
}

bool LocalRepository::LocalDirectoryExists() const {
  return fs::exists(GetFullPath());
}

void LocalRepository::CreateLocalDirectory() const {
  try {
    fs::create_directories(GetFullPath());
  } catch (const fs::filesystem_error& e) {
    std::cerr << "ERROR Creating Directory: " << e.what() << "\n";
    throw;
  }
}

void LocalRepository::RemoveLocalDirectory() const {
  try {
    fs::remove_all(GetFullPath());
  } catch (const fs::filesystem_error& e) {
    std::cerr << "ERROR Removing Directory: " << e.what() << "\n";
    throw;
  }
}
