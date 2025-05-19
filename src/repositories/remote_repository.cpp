#include "repositories/remote_repository.h"

#include <iostream>

RemoteRepository::RemoteRepository() {}

RemoteRepository::RemoteRepository(const std::string& path,
                                   const std::string& name,
                                   const std::string& password,
                                   const std::string& created_at) {
  path_ = path, name_ = name, password_ = password, created_at_ = created_at;
}

bool RemoteRepository::Exists() const {
  std::cout << " *** Checking Existence via SSH NOT Implemented Yet! *** "
            << std::endl;
  return false;
}

void RemoteRepository::Initialize() {
  std::cout << " *** Remote Repository Setup via SSH NOT Implemented Yet! *** "
            << std::endl;
}

void RemoteRepository::Delete() {
  std::cout << " *** Remote Repository Delete via SSH NOT Implemented Yet! *** "
            << std::endl;
}

void RemoteRepository::WriteConfigToRepo() const {
  std::cout << " *** Remote Repository WriteConfigToRepo via SSH NOT "
               "Implemented Yet! *** "
            << std::endl;
}

RemoteRepository RemoteRepository::FromConfigJson(const nlohmann::json& config) {
  std::string name = config.value("name", "");
  std::string path = config.value("path", "");
  std::string password_hash = config.value("password_hash", "");
  std::string created_at = config.value("created_at", "");

  RemoteRepository repo;
  repo.name_ = name;
  repo.path_ = path;
  repo.created_at_ = created_at;
  repo.password_ = "";  // For Security, NOT Storing Raw Password

  return repo;
}

std::string RemoteRepository::GetSFTPPath() const {
  return "sftp://" + name_ + ":" + password_ + "@" + path_;
}