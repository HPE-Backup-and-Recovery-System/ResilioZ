#include "repositories/local_repository.h"

#include <filesystem>
#include <fstream>
#include <iostream>
#include <nlohmann/json.hpp>

LocalRepository::LocalRepository() {}

LocalRepository::LocalRepository(const std::string& path,
                                 const std::string& name,
                                 const std::string& created_at)
    : path_(path), name_(name), created_at_(created_at) {}

bool LocalRepository::Exists() const {
  return std::filesystem::exists(path_ + "/" + name_);
}

void LocalRepository::Initialize() {
  std::string full_path = path_ + "/" + name_;
  std::filesystem::create_directories(full_path);

  nlohmann::json config = {{"name", name_}, {"type", "local"}, {"path", path_}, {"created_at", created_at_}};

  std::ofstream config_file(full_path + "/config.json");
  config_file << config.dump(4);
  config_file.close();
}

std::string LocalRepository::GetName() const { return name_; }
