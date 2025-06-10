#include "repositories/local_repository.h"

#include <filesystem>
#include <fstream>
#include <iostream>
#include <nlohmann/json.hpp>

#include "utils/error_util.h"

namespace fs = std::filesystem;

LocalRepository::LocalRepository() {}

LocalRepository::LocalRepository(const std::string& path,
                                 const std::string& name,
                                 const std::string& password,
                                 const std::string& created_at) {
  path_ = fs::weakly_canonical(fs::absolute(path));
  name_ = name;
  password_ = password;
  created_at_ = created_at;
  type_ = RepositoryType::LOCAL;
}

bool LocalRepository::UploadFile(const std::string& local_file,
                                 const std::string& local_path) const {
  if (!fs::exists(local_file)) {
    ErrorUtil::ThrowError("Source file not found at: " + local_file);
  }

  if (!fs::exists(local_path)) {
    fs::create_directories(local_path);
  }

  fs::path src_path(local_file);
  fs::path dest_path = fs::path(local_path) / src_path.filename();
  fs::copy_file(src_path, dest_path, fs::copy_options::overwrite_existing);
  return true;
}

bool LocalRepository::Exists() const { return LocalDirectoryExists(); }

void LocalRepository::Initialize() {
  CreateLocalDirectory();
  WriteConfig();
}

void LocalRepository::Delete() { RemoveLocalDirectory(); }

void LocalRepository::WriteConfig() const {
  std::string config_path = GetFullPath() + "/config.json";

  nlohmann::json config = {{"name", name_},
                           {"type", "local"},
                           {"path", path_},
                           {"created_at", created_at_},
                           {"password_hash", GetHashedPassword()}};

  std::ofstream file(config_path);
  if (!file.is_open()) {
    ErrorUtil::ThrowError("Failed to write config to: " + config_path);
  }

  file << config.dump(4);
  file.close();
}

LocalRepository LocalRepository::FromConfigJson(const nlohmann::json& config) {
  return LocalRepository(config.at("path"), config.at("name"),
                         config.value("password_hash", ""),
                         config.at("created_at"));
}

bool LocalRepository::LocalDirectoryExists() const {
  return fs::exists(GetFullPath());
}

void LocalRepository::CreateLocalDirectory() const {
  fs::create_directories(GetFullPath());
}

void LocalRepository::RemoveLocalDirectory() const {
  fs::remove_all(GetFullPath());
}
