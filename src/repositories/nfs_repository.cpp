#include "repositories/nfs_repository.h"

#include <filesystem>
#include <fstream>
#include <nlohmann/json.hpp>

#include "utils/error_util.h"

namespace fs = std::filesystem;

NFSRepository::NFSRepository() {}

NFSRepository::NFSRepository(const std::string& nfs_mount_path,
                             const std::string& name,
                             const std::string& password,
                             const std::string& created_at) {
  path_ = path_ = fs::weakly_canonical(fs::absolute(nfs_mount_path));
  name_ = name;
  password_ = password;
  created_at_ = created_at;
  type_ = RepositoryType::NFS;
}

bool NFSRepository::UploadFile(const std::string& local_file,
                               const std::string& remote_path) const {
  if (!fs::exists(local_file)) {
    ErrorUtil::ThrowError("Source file not found: " + local_file);
  }

  if (!fs::exists(remote_path)) {
    fs::create_directories(remote_path);
  }

  fs::path src_path(local_file);
  fs::path dest_path = fs::path(remote_path) / src_path.filename();
  fs::copy_file(src_path, dest_path, fs::copy_options::overwrite_existing);

  return true;
}

bool NFSRepository::Exists() const {
  return NFSMountExists() && fs::exists(GetFullPath());
}

void NFSRepository::Initialize() {
  EnsureNFSMounted();
  CreateRemoteDirectory();
  WriteConfig();
}

void NFSRepository::Delete() { RemoveRemoteDirectory(); }

void NFSRepository::WriteConfig() const {
  std::string config_path = GetFullPath() + "/config.json";

  nlohmann::json config = {{"name", name_},
                           {"type", "nfs"},
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

NFSRepository NFSRepository::FromConfigJson(const nlohmann::json& config) {
  return NFSRepository(config.at("path"), config.at("name"),
                       config.value("password_hash", ""),
                       config.at("created_at"));
}

bool NFSRepository::NFSMountExists() const { return fs::exists(path_); }

void NFSRepository::EnsureNFSMounted() const {
  if (!NFSMountExists()) {
    ErrorUtil::ThrowError("NFS mount path not available: " + path_);
  }
}

void NFSRepository::CreateRemoteDirectory() const {
  fs::create_directories(GetFullPath());
}

void NFSRepository::RemoveRemoteDirectory() const {
  fs::remove_all(GetFullPath());
}