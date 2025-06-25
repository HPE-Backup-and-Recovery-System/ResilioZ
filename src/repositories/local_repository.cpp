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

bool LocalRepository::UploadFile(const std::string& local_file,
                                 const std::string& local_path) const {
  std::string local_full_path;
  try {
    if (!fs::exists(local_file)) {
      ErrorUtil::ThrowError("File does not exist: " + local_file);
    }

    fs::path local_fs_path(local_file);
    std::string filename = local_fs_path.filename().string();

    if (local_path.empty()) {
      local_full_path = path_ + "/" + filename;
    } else {
      if (local_path.back() == '/') {
        local_full_path = path_ + "/" + local_path + filename;
      } else {
        local_full_path = path_ + "/" + local_path + "/" + filename;
      }
    }

    if (!fs::exists(local_path)) {
      fs::create_directories(local_path);
    }

    fs::path src_path(local_file), dest_path(local_full_path);
    fs::copy_file(src_path, dest_path, fs::copy_options::overwrite_existing);
    return true;

  } catch (const std::exception& e) {
    ErrorUtil::ThrowNested("Cannot upload file to local path: " +
                           local_full_path);
  }
  return false;
}

bool LocalRepository::UploadDirectory(const std::string& local_dir,
                                      const std::string& local_path) const {
  std::string local_full_path;

  try {
    fs::path src(local_dir);
    if (!fs::exists(src) || !fs::is_directory(src)) {
      ErrorUtil::ThrowError("Directory does not exist: " + local_dir);
    }

    std::string dirname = src.filename().string();

    if (local_path.empty()) {
      local_full_path = path_ + "/" + dirname;
    } else {
      if (local_path.back() == '/') {
        local_full_path = path_ + "/" + local_path + dirname;
      } else {
        local_full_path = path_ + "/" + local_path + "/" + dirname;
      }
    }

    fs::path dest(local_full_path);
    fs::create_directories(dest);

    for (const auto& entry : fs::recursive_directory_iterator(src)) {
      const auto& rel_path = fs::relative(entry.path(), src);
      fs::path target_path = dest / rel_path;

      if (fs::is_directory(entry.status())) {
        fs::create_directories(target_path);
      } else if (fs::is_regular_file(entry.status())) {
        fs::create_directories(target_path.parent_path());
        fs::copy_file(entry.path(), target_path,
                      fs::copy_options::overwrite_existing);
      }
    }

    return true;

  } catch (const std::exception& e) {
    ErrorUtil::ThrowNested("Cannot upload directory to local path: " +
                           local_full_path);
  }
  return false;
}

bool LocalRepository::DownloadFile(const std::string& local_file,
                                   const std::string& local_path) const {
  std::string repo_full_path = path_ + "/" + local_file;

  try {
    fs::path repo_fs_path(repo_full_path);
    if (!fs::exists(repo_fs_path) || !fs::is_regular_file(repo_fs_path)) {
      ErrorUtil::ThrowError("Source file not found: " + repo_full_path);
    }

    std::string filename = repo_fs_path.filename().string();
    std::string local_full_path;

    if (local_path.empty()) {
      local_full_path = filename;
    } else {
      fs::path local_fs_path(local_path);
      if (fs::is_directory(local_fs_path) || local_path.back() == '/') {
        local_full_path = (local_fs_path / filename).string();
      } else {
        local_full_path = local_path;
      }
    }

    fs::create_directories(fs::path(local_full_path).parent_path());
    fs::copy_file(repo_fs_path, local_full_path,
                  fs::copy_options::overwrite_existing);
    return true;

  } catch (...) {
    ErrorUtil::ThrowNested("Cannot download file from local repository: " +
                           repo_full_path);
  }
  return false;
}

bool LocalRepository::DownloadDirectory(const std::string& local_dir,
                                        const std::string& local_path) const {
  std::string repo_root = path_ + "/" + local_dir;

  try {
    fs::path src_root(repo_root);
    if (!fs::exists(src_root) || !fs::is_directory(src_root)) {
      ErrorUtil::ThrowError("Directory not found in repository: " + repo_root);
    }

    std::string dirname = src_root.filename().string();
    fs::path dest_root;

    if (local_path.empty()) {
      dest_root = fs::path(".") / dirname;
    } else {
      fs::path local_fs_path(local_path);
      dest_root = (local_fs_path / dirname);
    }

    std::function<void(const fs::path&, const fs::path&)> copy_recursive;
    copy_recursive = [&](const fs::path& src, const fs::path& dst) {
      fs::create_directories(dst);

      for (const auto& entry : fs::directory_iterator(src)) {
        const fs::path& src_path = entry.path();
        fs::path dst_path = dst / src_path.filename();

        if (fs::is_directory(src_path)) {
          copy_recursive(src_path, dst_path);
        } else if (fs::is_regular_file(src_path)) {
          fs::create_directories(dst_path.parent_path());
          fs::copy_file(src_path, dst_path,
                        fs::copy_options::overwrite_existing);
        }
      }
    };

    copy_recursive(src_root, dest_root);
    return true;

  } catch (...) {
    ErrorUtil::ThrowNested("Cannot download directory from local repository: " +
                           repo_root);
  }
  return false;
}
