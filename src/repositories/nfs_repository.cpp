#include "repositories/nfs_repository.h"

#include <fcntl.h>
#include <nfsc/libnfs.h>
#include <sys/stat.h>

#include <filesystem>
#include <fstream>
#include <nlohmann/json.hpp>

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
  if (colon_pos == std::string::npos || colon_pos == 0 ||
      colon_pos == nfs_path.size() - 1) {
    ErrorUtil::ThrowError("Invalid NFS path format. Expected: ip:/path");
  }
  server_ip_ = nfs_path.substr(0, colon_pos);
  server_backup_path_ = nfs_path.substr(colon_pos + 1);
  if (server_backup_path_.empty() || server_backup_path_[0] != '/') {
    ErrorUtil::ThrowError("NFS path must be absolute (start with /)");
  }
}

bool NFSRepository::UploadFile(const std::string& local_file,
                               const std::string& remote_path) const {
  std::string repo_dir = "/" + name_;
  std::string remote_file_path = repo_dir + "/" +
                                 (!remote_path.empty() ? (remote_path) : "") +
                                 fs::path(local_file).filename().string();

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
      ErrorUtil::ThrowError("Remote file create failed: " + remote_file_path +
                            " - " + err);
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
    ErrorUtil::ThrowNested("Cannot upload file to remote NFS path: " +
                           remote_file_path);
  }
}

bool NFSRepository::UploadDirectory(const std::string& local_dir,
                                    const std::string& remote_path) const {
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
    upload_recursive = [&](const fs::path& path,
                           const std::string& remote_dir) {
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
        while (infile.read(buffer.data(), buffer.size()) ||
               infile.gcount() > 0) {
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
    ErrorUtil::ThrowNested("Cannot upload directory to remote NFS path: " +
                           remote_base);
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
  std::string backup_dir = "/" + name_ + "/backup";
  if (nfs_mkdir(nfs, backup_dir.c_str()) < 0) {
    std::string err = nfs_get_error(nfs);
    if (err.find("exists") == std::string::npos) {
      nfs_umount(nfs);
      nfs_destroy_context(nfs);
      ErrorUtil::ThrowError("mkdir failed: " + err);
    }
  }
  std::string chunk_dir = "/" + name_ + "/chunks";
  if (nfs_mkdir(nfs, chunk_dir.c_str()) < 0) {
    std::string err = nfs_get_error(nfs);
    if (err.find("exists") == std::string::npos) {
      nfs_umount(nfs);
      nfs_destroy_context(nfs);
      ErrorUtil::ThrowError("mkdir failed: " + err);
    }
  }
  std::vector<std::string> hexArray = {
      "00", "01", "02", "03", "04", "05", "06", "07", "08", "09", "0a", "0b",
      "0c", "0d", "0e", "0f", "10", "11", "12", "13", "14", "15", "16", "17",
      "18", "19", "1a", "1b", "1c", "1d", "1e", "1f", "20", "21", "22", "23",
      "24", "25", "26", "27", "28", "29", "2a", "2b", "2c", "2d", "2e", "2f",
      "30", "31", "32", "33", "34", "35", "36", "37", "38", "39", "3a", "3b",
      "3c", "3d", "3e", "3f", "40", "41", "42", "43", "44", "45", "46", "47",
      "48", "49", "4a", "4b", "4c", "4d", "4e", "4f", "50", "51", "52", "53",
      "54", "55", "56", "57", "58", "59", "5a", "5b", "5c", "5d", "5e", "5f",
      "60", "61", "62", "63", "64", "65", "66", "67", "68", "69", "6a", "6b",
      "6c", "6d", "6e", "6f", "70", "71", "72", "73", "74", "75", "76", "77",
      "78", "79", "7a", "7b", "7c", "7d", "7e", "7f", "80", "81", "82", "83",
      "84", "85", "86", "87", "88", "89", "8a", "8b", "8c", "8d", "8e", "8f",
      "90", "91", "92", "93", "94", "95", "96", "97", "98", "99", "9a", "9b",
      "9c", "9d", "9e", "9f", "a0", "a1", "a2", "a3", "a4", "a5", "a6", "a7",
      "a8", "a9", "aa", "ab", "ac", "ad", "ae", "af", "b0", "b1", "b2", "b3",
      "b4", "b5", "b6", "b7", "b8", "b9", "ba", "bb", "bc", "bd", "be", "bf",
      "c0", "c1", "c2", "c3", "c4", "c5", "c6", "c7", "c8", "c9", "ca", "cb",
      "cc", "cd", "ce", "cf", "d0", "d1", "d2", "d3", "d4", "d5", "d6", "d7",
      "d8", "d9", "da", "db", "dc", "dd", "de", "df", "e0", "e1", "e2", "e3",
      "e4", "e5", "e6", "e7", "e8", "e9", "ea", "eb", "ec", "ed", "ee", "ef",
      "f0", "f1", "f2", "f3", "f4", "f5", "f6", "f7", "f8", "f9", "fa", "fb",
      "fc", "fd", "fe", "ff"};
  for (auto i : hexArray) {
    if (nfs_mkdir(nfs, (chunk_dir + "/" + i).c_str()) < 0) {
      std::string err = nfs_get_error(nfs);
      if (err.find("exists") == std::string::npos) {
        nfs_umount(nfs);
        nfs_destroy_context(nfs);
        ErrorUtil::ThrowError("mkdir failed: " + err);
      }
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
  std::string config_path = repo_dir + "/config.json";
  nfs_unlink(nfs, config_path.c_str());
  auto recursive_delete = [](struct nfs_context* ctx, const std::string& path, auto&& self_ref) -> void {
  struct nfsdir *dir;
  struct nfsdirent *entry;

  if (nfs_opendir(ctx, path.c_str(), &dir) < 0) return;

  while ((entry = nfs_readdir(ctx, dir)) != nullptr) {
    std::string name = entry->name;
    if (name == "." || name == "..") continue;

    std::string full_path = path + "/" + name;

    struct nfs_stat_64 st;
    if (nfs_stat64(ctx, full_path.c_str(), &st) < 0) continue;

    if (S_ISDIR(st.nfs_mode)) {
      self_ref(ctx, full_path, self_ref);        
      nfs_rmdir(ctx, full_path.c_str());          
    } else {
      nfs_unlink(ctx, full_path.c_str());         
    }
  }
  nfs_closedir(ctx, dir);
  };
  recursive_delete(nfs, repo_dir, recursive_delete);
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
  fs::path temp_file = fs::temp_directory_path() / "config.json";
  std::ofstream out(temp_file);
  if (!out) {
    ErrorUtil::ThrowError("Failed to create temporary config file");
  }
  out << config.dump(4);
  out.close();
  if (!UploadFile(temp_file)) {
    ErrorUtil::ThrowError("Failed to upload config to NFS");
  }
  fs::remove(temp_file);
}

NFSRepository NFSRepository::FromConfigJson(const nlohmann::json& config) {
  return NFSRepository(config.at("path"), config.at("name"),
                       config.value("password_hash", ""),
                       config.at("created_at"));
}

std::vector<std::string> NFSRepository::ListFiles(
    const std::string& remote_dir) const {
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

bool NFSRepository::DownloadFile(const std::string& remote_file,
                                 const std::string& local_file) const {
  std::string repo_dir = "/" + name_;
  std::string remote_full_path =
      remote_file.empty() ? repo_dir : (repo_dir + "/" + remote_file);

  struct nfs_context* nfs = nfs_init_context();
  if (!nfs) ErrorUtil::ThrowError("Failed to init NFS context");
  try {
    fs::path remote_fs_path(remote_file);
    std::string filename = remote_fs_path.filename().string();

    std::string local_full_path;
    if (local_file.empty()) {
      local_full_path = filename;
    } else {
      fs::path local_fs_path(local_file);
      if (fs::is_directory(local_fs_path) || local_file.back() == '/') {
        local_full_path = (local_fs_path / filename).string();
      } else {
        local_full_path = local_file;
      }
    }

    if (nfs_mount(nfs, server_ip_.c_str(), server_backup_path_.c_str()) < 0) {
      std::string err = nfs_get_error(nfs);
      nfs_destroy_context(nfs);
      ErrorUtil::ThrowError("Mount failed: " + err);
    }

    struct nfsfh* fh;
    if (nfs_open(nfs, remote_full_path.c_str(), O_RDONLY, &fh) < 0) {
      nfs_umount(nfs);
      nfs_destroy_context(nfs);
      ErrorUtil::ThrowError("Unable to open remote file for reading: " +
                            remote_full_path);
    }

    std::ofstream output(local_full_path, std::ios::binary);
    if (!output) {
      nfs_close(nfs, fh);
      nfs_umount(nfs);
      nfs_destroy_context(nfs);
      ErrorUtil::ThrowError("Failed to open local file for writing: " +
                            local_full_path);
    }

    char buffer[1048576];
    ssize_t n;
    while ((n = nfs_read(nfs, fh, sizeof(buffer), buffer)) > 0) {
      output.write(buffer, n);
    }

    if (n < 0) {
      nfs_close(nfs, fh);
      nfs_umount(nfs);
      nfs_destroy_context(nfs);
      ErrorUtil::ThrowError("Error reading from remote file: " +
                            remote_full_path);
    }

    nfs_close(nfs, fh);
    nfs_umount(nfs);
    nfs_destroy_context(nfs);
    return true;

  } catch (...) {
    nfs_destroy_context(nfs);
    ErrorUtil::ThrowNested("Cannot download file from remote NFS path: " +
                           remote_full_path);
  }
}

bool NFSRepository::DownloadDirectory(const std::string& remote_dir,
                                      const std::string& local_path) const {
  std::string repo_dir = "/" + name_;
  std::string remote_root =
      remote_dir.empty() ? repo_dir : (repo_dir + "/" + remote_dir);

  struct nfs_context* nfs = nfs_init_context();
  if (!nfs) ErrorUtil::ThrowError("Failed to init NFS context");
  try {
    if (!fs::exists(local_path)) {
      fs::create_directories(local_path);
    }

    if (nfs_mount(nfs, server_ip_.c_str(), server_backup_path_.c_str()) < 0) {
      std::string err = nfs_get_error(nfs);
      nfs_destroy_context(nfs);
      ErrorUtil::ThrowError("Mount failed: " + err);
    }

    std::function<void(const std::string&, const fs::path&)> download_recursive;
    download_recursive = [&](const std::string& remote_subpath,
                             const fs::path& local_subdir) {
      struct nfsdir* dir;
      if (nfs_opendir(nfs, remote_subpath.c_str(), &dir) < 0) {
        ErrorUtil::ThrowError("Cannot open remote directory: " +
                              remote_subpath);
      }

      fs::create_directories(local_subdir);

      struct nfsdirent* entry;
      while ((entry = nfs_readdir(nfs, dir)) != nullptr) {
        std::string name = entry->name;
        if (name == "." || name == "..") {
          continue;
        }

        std::string full_remote = remote_subpath + "/" + name;
        fs::path full_local = local_subdir / name;

        struct nfs_stat_64 st;
        if (nfs_stat64(nfs, full_remote.c_str(), &st) == 0) {
          if (S_ISDIR(st.nfs_mode)) {
            download_recursive(full_remote, full_local);
          } else if (S_ISREG(st.nfs_mode)) {
            struct nfsfh* fh;
            if (nfs_open(nfs, full_remote.c_str(), O_RDONLY, &fh) < 0) {
              continue;
            }

            std::ofstream output(full_local, std::ios::binary);
            if (!output) {
              nfs_close(nfs, fh);
              continue;
            }

            char buffer[1048576];
            ssize_t n;
            while ((n = nfs_read(nfs, fh, sizeof(buffer), buffer)) > 0) {
              output.write(buffer, n);
            }

            nfs_close(nfs, fh);
          }
        }
      }

      nfs_closedir(nfs, dir);
    };

    download_recursive(remote_root, fs::path(local_path));

    nfs_umount(nfs);
    nfs_destroy_context(nfs);
    return true;

  } catch (...) {
    nfs_destroy_context(nfs);
    ErrorUtil::ThrowNested("Cannot download directory from remote NFS path: " +
                           remote_root);
  }
}