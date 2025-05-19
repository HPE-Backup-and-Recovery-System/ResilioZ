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

bool LocalRepository::Exists() const { return fs::exists(GetFullPath()); }

void LocalRepository::Initialize() {
  std::string full_path = GetFullPath();
  fs::create_directories(full_path);
  WriteConfigToRepo();
}

void LocalRepository::Delete() {
  std::string full_path = GetFullPath();
  fs::remove_all(full_path);
}

std::string LocalRepository::GetFullPath() const { return path_ + "/" + name_; }

void LocalRepository::WriteConfigToRepo() const {
  std::string config_path = GetFullPath() + "/config.json";

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
}

LocalRepository LocalRepository::FromConfigJson(const nlohmann::json& config) {
  std::string name = config.value("name", "");
  std::string path = config.value("path", "");
  std::string password_hash = config.value("password_hash", "");
  std::string created_at = config.value("created_at", "");

  LocalRepository repo;
  repo.name_ = name;
  repo.path_ = path;
  repo.created_at_ = created_at;
  repo.password_ = "";  // For Security, NOT Storing Raw Password

  return repo;
}