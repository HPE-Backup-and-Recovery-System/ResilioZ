#include "repositories/repository.h"

#include <openssl/sha.h>

#include <filesystem>
#include <iomanip>
#include <sstream>

#include "utils/repodata_manager.h"
#include "utils/validator.h"

namespace fs = std::filesystem;

std::string Repository::GetName() const { return name_; }

std::string Repository::GetPath() const { return path_; }

std::string Repository::GetFullPath() const { return path_ + "/" + name_; }

std::string Repository::GetPassword() const { return password_; }

RepositoryType Repository::GetType() const { return type_; }

std::string Repository::GetRepositoryInfoString() const {
  return name_ + " [" + GetFormattedTypeString(type_) + "] - " + path_;
}

std::string Repository::GetRepositoryInfoString(const std::string& name,
                                                const std::string& type,
                                                const std::string& path) {
  return name + " [" + GetFormattedTypeString(type) + "] - " + path;
}

std::string Repository::GetRepositoryInfoString(const std::string& name,
                                                const RepositoryType& type,
                                                const std::string& path) {
  return name + " [" + GetFormattedTypeString(type) + "] - " + path;
}

std::string Repository::GetHashedPassword() const {
  return GetHashedPassword(password_);
}

std::string Repository::GetHashedPassword(const std::string& password) {
  unsigned char hash[SHA256_DIGEST_LENGTH];
  SHA256(reinterpret_cast<const unsigned char*>(password.c_str()),
         password.size(), hash);

  std::ostringstream oss;
  for (int i = 0; i < SHA256_DIGEST_LENGTH; ++i) {
    oss << std::hex << std::setw(2) << std::setfill('0')
        << static_cast<int>(hash[i]);
  }

  return oss.str();
}

std::string Repository::GetResolvedPath(const std::string& path) {
  return Validator::IsValidSftpPath(path)
             ? path
             : fs::weakly_canonical(fs::absolute(path)).string();
}

std::string Repository::GetFormattedTypeString(const std::string& type,
                                               bool upper) {
  if (type == "local") return (upper ? "LOCAL" : "Local");
  if (type == "nfs") return "NFS";
  if (type == "remote") return (upper ? "REMOTE" : "Remote");
  return (upper ? "UNKNOWN" : "Unknown");
}

std::string Repository::GetFormattedTypeString(const RepositoryType& type,
                                               bool upper) {
  if (type == RepositoryType::LOCAL) return (upper ? "LOCAL" : "Local");
  if (type == RepositoryType::NFS) return "NFS";
  if (type == RepositoryType::REMOTE) return (upper ? "REMOTE" : "Remote");
  return (upper ? "UNKNOWN" : "Unknown");
}
