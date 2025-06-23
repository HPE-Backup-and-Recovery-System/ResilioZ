#include "repositories/nfs_repository.h"

#include <filesystem>
#include <fstream>
#include <nlohmann/json.hpp>
#include <nfsc/libnfs.h>
#include <fcntl.h>

#include "utils/error_util.h"

namespace fs = std::filesystem;

NFSRepository::NFSRepository() {}

NFSRepository::NFSRepository(const std::string& nfs_path,
                             const std::string& name,
                             const std::string& password,
                             const std::string& created_at) {
  name_ = name;
  path_ = nfs_path;
  password_ = password;
  created_at_ = created_at;
  type_ = RepositoryType::NFS;
  ParseNfsPath(nfs_path);
}

void NFSRepository::ParseNfsPath(const std::string& nfs_path) {
  auto colon_pos = nfs_path.find(":");
  if (colon_pos == std::string::npos || colon_pos == 0 || colon_pos == nfs_path.size() - 1) {
    ErrorUtil::ThrowError("Invalid NFS path format. Expected: ip:/path");
  }
  server_ip_ = nfs_path.substr(0, colon_pos);
  server_backup_path_ = nfs_path.substr(colon_pos + 1);
  if (server_backup_path_.empty() || server_backup_path_[0] != '/') {
    ErrorUtil::ThrowError("NFS path must be absolute (start with /)");
  }
}

void NFSRepository::CreateRemoteDirectory() const {
  struct nfs_context* nfs = nfs_init_context();
  if (!nfs) ErrorUtil::ThrowError("Failed to init NFS context");
  try {
    if (nfs_mount(nfs, server_ip_.c_str(), server_backup_path_.c_str()) < 0) {
      std::string err = nfs_get_error(nfs);
      nfs_destroy_context(nfs);
      ErrorUtil::ThrowError("Mount failed: " + err);
    }
    std::string repo_dir = "/" + name_;
    if (nfs_mkdir(nfs, repo_dir.c_str()) < 0) {
      std::string err = nfs_get_error(nfs);
      if (err.find("exists") == std::string::npos) {
        nfs_umount(nfs);
        nfs_destroy_context(nfs);
        ErrorUtil::ThrowError("mkdir failed: " + err);
      }
    }
    nfs_umount(nfs);
    nfs_destroy_context(nfs);
  } catch (...) {
    nfs_destroy_context(nfs);
    ErrorUtil::ThrowNested("Failed to create remote NFS directory");
  }
}

void NFSRepository::RemoveRemoteDirectory() const {
  struct nfs_context* nfs = nfs_init_context();
  if (!nfs) ErrorUtil::ThrowError("Failed to init NFS context");
  try {
    if (nfs_mount(nfs, server_ip_.c_str(), server_backup_path_.c_str()) < 0) {
      std::string err = nfs_get_error(nfs);
      nfs_destroy_context(nfs);
      ErrorUtil::ThrowError("Mount failed: " + err);
    }
    std::string repo_dir = "/" + name_;
    std::string config_path = repo_dir + "/config.json";
    nfs_unlink(nfs, config_path.c_str());
    nfs_rmdir(nfs, repo_dir.c_str());
    nfs_umount(nfs);
    nfs_destroy_context(nfs);
  } catch (...) {
    nfs_destroy_context(nfs);
    ErrorUtil::ThrowNested("Failed to remove remote NFS directory");
  }
}

bool NFSRepository::NFSMountExists() const {
  struct nfs_context* nfs = nfs_init_context();
  if (!nfs) return false;
  bool mounted = nfs_mount(nfs, server_ip_.c_str(), server_backup_path_.c_str()) == 0;
  if (mounted) nfs_umount(nfs);
  nfs_destroy_context(nfs);
  return mounted;
}

bool NFSRepository::UploadFile(const std::string& local_file, const std::string& remote_path) const {
  std::string repo_dir = "/" + name_;
  std::string remote_file_path = remote_path.empty() ? (repo_dir + "/" + fs::path(local_file).filename().string()) : remote_path;

  struct nfs_context* nfs = nfs_init_context();
  if (!nfs) ErrorUtil::ThrowError("Failed to init NFS context");
  try {
    if (nfs_mount(nfs, server_ip_.c_str(), server_backup_path_.c_str()) < 0) {
      std::string err = nfs_get_error(nfs);
      nfs_destroy_context(nfs);
      ErrorUtil::ThrowError("Mount failed: " + err);
    }
    std::ifstream file(local_file, std::ios::binary);
    if (!file) {
      nfs_umount(nfs);
      nfs_destroy_context(nfs);
      ErrorUtil::ThrowError("Cannot open local file: " + local_file);
    }
    struct nfsfh* fh;
    if (nfs_creat(nfs, remote_file_path.c_str(), 0644, &fh) < 0) {
      std::string err = nfs_get_error(nfs);
      nfs_umount(nfs);
      nfs_destroy_context(nfs);
      ErrorUtil::ThrowError("Remote file create failed: " + remote_file_path + " - " + err);
    }
    std::vector<char> buffer(1048576);
    ssize_t offset = 0;
    while (file.read(buffer.data(), buffer.size()) || file.gcount() > 0) {
      ssize_t len = file.gcount();
      if (nfs_pwrite(nfs, fh, offset, len, buffer.data()) < 0) {
        std::string err = nfs_get_error(nfs);
        nfs_close(nfs, fh);
        nfs_umount(nfs);
        nfs_destroy_context(nfs);
        ErrorUtil::ThrowError("Write failed: " + err);
      }
      offset += len;
    }
    nfs_close(nfs, fh);
    nfs_umount(nfs);
    nfs_destroy_context(nfs);
    return true;
  } catch (...) {
    nfs_destroy_context(nfs);
    ErrorUtil::ThrowNested("Cannot upload file to remote NFS path: " + remote_file_path);
  }
}

bool NFSRepository::UploadDirectory(const std::string& local_dir, const std::string& remote_path) const {
  std::string repo_dir = "/" + name_;
  std::string remote_base = remote_path.empty() ? repo_dir : remote_path;
  struct nfs_context* nfs = nfs_init_context();
  if (!nfs) ErrorUtil::ThrowError("Failed to init NFS context");
  try {
    if (nfs_mount(nfs, server_ip_.c_str(), server_backup_path_.c_str()) < 0) {
      std::string err = nfs_get_error(nfs);
      nfs_destroy_context(nfs);
      ErrorUtil::ThrowError("Mount failed: " + err);
    }
    std::function<void(const fs::path&, const std::string&)> upload_recursive;
    upload_recursive = [&](const fs::path& path, const std::string& remote_dir) {
      if (fs::is_directory(path)) {
        std::string dir_name = path.filename().string();
        std::string remote_subdir = remote_dir + "/" + dir_name;
        nfs_mkdir(nfs, remote_subdir.c_str());
        for (const auto& entry : fs::directory_iterator(path)) {
          upload_recursive(entry.path(), remote_subdir);
        }
      } else if (fs::is_regular_file(path)) {
        std::string file_name = path.filename().string();
        std::string remote_file = remote_dir + "/" + file_name;
        std::ifstream infile(path, std::ios::binary);
        if (!infile) return;
        struct nfsfh* fh;
        if (nfs_creat(nfs, remote_file.c_str(), 0644, &fh) < 0) {
          return;
        }
        std::vector<char> buffer(1048576);
        ssize_t offset = 0;
        while (infile.read(buffer.data(), buffer.size()) || infile.gcount() > 0) {
          ssize_t len = infile.gcount();
          if (nfs_pwrite(nfs, fh, offset, len, buffer.data()) < 0) {
            break;
          }
          offset += len;
        }
        nfs_close(nfs, fh);
      }
    };
    for (const auto& entry : fs::directory_iterator(local_dir)) {
      upload_recursive(entry.path(), remote_base);
    }
    nfs_umount(nfs);
    nfs_destroy_context(nfs);
  return true;
  } catch (...) {
    nfs_destroy_context(nfs);
    ErrorUtil::ThrowNested("Cannot upload directory to remote NFS path: " + remote_base);
  }
}

bool NFSRepository::Exists() const {
  struct nfs_context* nfs = nfs_init_context();
  if (!nfs) return false;
  if (nfs_mount(nfs, server_ip_.c_str(), server_backup_path_.c_str()) < 0) {
    nfs_destroy_context(nfs);
    return false;
  }
  std::string repo_dir = "/" + name_;
  struct nfs_stat_64 st;
  bool exists = nfs_stat64(nfs, repo_dir.c_str(), &st) == 0;
  nfs_umount(nfs);
  nfs_destroy_context(nfs);
  return exists;
}

void NFSRepository::Initialize() {
  struct nfs_context* nfs = nfs_init_context();
  if (!nfs) ErrorUtil::ThrowError("Failed to init NFS context");
  if (nfs_mount(nfs, server_ip_.c_str(), server_backup_path_.c_str()) < 0) {
    std::string err = nfs_get_error(nfs);
    nfs_destroy_context(nfs);
    ErrorUtil::ThrowError("Mount failed: " + err);
  }
  // Create directory for this repo (relative to export root)
  std::string repo_dir = "/" + name_;
  if (nfs_mkdir(nfs, repo_dir.c_str()) < 0) {
    std::string err = nfs_get_error(nfs);
    if (err.find("exists") == std::string::npos) {
      nfs_umount(nfs);
      nfs_destroy_context(nfs);
      ErrorUtil::ThrowError("mkdir failed: " + err);
    }
  }
  nfs_umount(nfs);
  nfs_destroy_context(nfs);
  WriteConfig();
}

void NFSRepository::Delete() {
  struct nfs_context* nfs = nfs_init_context();
  if (!nfs) ErrorUtil::ThrowError("Failed to init NFS context");
  if (nfs_mount(nfs, server_ip_.c_str(), server_backup_path_.c_str()) < 0) {
    std::string err = nfs_get_error(nfs);
    nfs_destroy_context(nfs);
    ErrorUtil::ThrowError("Mount failed: " + err);
  }
  std::string repo_dir = "/" + name_;
  // Remove config.json first
  std::string config_path = repo_dir + "/config.json";
  nfs_unlink(nfs, config_path.c_str());
  // Remove the directory
  nfs_rmdir(nfs, repo_dir.c_str());
  nfs_umount(nfs);
  nfs_destroy_context(nfs);
}

void NFSRepository::WriteConfig() const {
  nlohmann::json config = {{"name", name_},
                           {"type", "nfs"},
                           {"path", path_},
                           {"created_at", created_at_},
                           {"password_hash", GetHashedPassword()},
                           {"server_ip", server_ip_},
                           {"server_backup_path", server_backup_path_}};
  std::string temp_file = "/tmp/repo_config_temp.json";
  std::ofstream out(temp_file);
  if (!out) {
    ErrorUtil::ThrowError("Failed to create temporary config file");
  }
  out << config.dump(4);
  out.close();
  std::string remote_config_path = "/" + name_ + "/config.json";
  if (!UploadFile(temp_file, remote_config_path)) {
    ErrorUtil::ThrowError("Failed to upload config to NFS");
  }
  fs::remove(temp_file);
}

NFSRepository NFSRepository::FromConfigJson(const nlohmann::json& config) {
  return NFSRepository(config.at("path"), config.at("name"),
                       config.value("password_hash", ""),
                       config.at("created_at"));
}

std::vector<std::string> NFSRepository::ListFiles(const std::string& remote_dir) const {
  std::vector<std::string> files;
  struct nfs_context* nfs = nfs_init_context();
  if (!nfs) ErrorUtil::ThrowError("Failed to init NFS context");
  if (nfs_mount(nfs, server_ip_.c_str(), server_backup_path_.c_str()) < 0) {
    std::string err = nfs_get_error(nfs);
    nfs_destroy_context(nfs);
    ErrorUtil::ThrowError("Mount failed: " + err);
  }
  struct nfsdir* dir;
  std::string dir_path = remote_dir.empty() ? ("/" + name_) : remote_dir;
  if (nfs_opendir(nfs, dir_path.c_str(), &dir) < 0) {
    nfs_umount(nfs);
    nfs_destroy_context(nfs);
    return files;
  }
  struct nfsdirent* entry;
  while ((entry = nfs_readdir(nfs, dir)) != nullptr) {
    std::string fname = entry->name;
    if (fname != "." && fname != "..") {
      files.push_back(fname);
    }
  }
  nfs_closedir(nfs, dir);
  nfs_umount(nfs);
  nfs_destroy_context(nfs);
  return files;
}

bool NFSRepository::DownloadFile(const std::string& remote_file, const std::string& local_file) const {
  struct nfs_context* nfs = nfs_init_context();
  if (!nfs) ErrorUtil::ThrowError("Failed to init NFS context");
  if (nfs_mount(nfs, server_ip_.c_str(), server_backup_path_.c_str()) < 0) {
    std::string err = nfs_get_error(nfs);
    nfs_destroy_context(nfs);
    ErrorUtil::ThrowError("Mount failed: " + err);
  }
  struct nfsfh* fh;
  if (nfs_open(nfs, remote_file.c_str(), O_RDONLY, &fh) < 0) {
    nfs_umount(nfs);
    nfs_destroy_context(nfs);
    return false;
  }
  std::ofstream out(local_file, std::ios::binary);
  if (!out) {
    nfs_close(nfs, fh);
    nfs_umount(nfs);
    nfs_destroy_context(nfs);
    return false;
  }
  char buffer[1048576];
  ssize_t n;
  while ((n = nfs_read(nfs, fh, sizeof(buffer), buffer)) > 0) {
    out.write(buffer, n);
  }
  nfs_close(nfs, fh);
  nfs_umount(nfs);
  nfs_destroy_context(nfs);
  return true;
}