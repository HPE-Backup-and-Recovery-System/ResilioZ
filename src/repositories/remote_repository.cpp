#include "repositories/remote_repository.h"

#include <fcntl.h>
#include <libssh/libssh.h>
#include <libssh/sftp.h>
#include <sys/stat.h>

#include <filesystem>
#include <fstream>
#include <iostream>
#include <sstream>
#include <stdexcept>

#include "utils/error_util.h"

namespace fs = std::filesystem;

RemoteRepository::RemoteRepository() {}

RemoteRepository::RemoteRepository(const std::string& sftp_path,
                                   const std::string& name,
                                   const std::string& password,
                                   const std::string& created_at) {
  name_ = name;
  path_ = sftp_path;
  password_ = password;
  created_at_ = created_at;
  type_ = RepositoryType::REMOTE;

  ParseSFTPPath(sftp_path);
}

void RemoteRepository::ParseSFTPPath(const std::string& sftp_path) {
  auto at_pos = sftp_path.find('@');
  auto colon_pos = sftp_path.find(':');

  if (at_pos == std::string::npos || colon_pos == std::string::npos ||
      colon_pos < at_pos) {
    ErrorUtil::ThrowError(
        "Invalid SFTP path format. Expected: user@host:/path");
  }

  user_ = sftp_path.substr(0, at_pos);
  host_ = sftp_path.substr(at_pos + 1, colon_pos - at_pos - 1);
  remote_dir_ = sftp_path.substr(colon_pos + 1);

  if (remote_dir_.back() != '/') remote_dir_.push_back('/');
  remote_dir_ += name_;
}

bool RemoteRepository::Exists() const { return RemoteDirectoryExists(); }

void RemoteRepository::Initialize() {
  CreateRemoteDirectory();
  WriteConfig();
}

void RemoteRepository::Delete() { RemoveRemoteDirectory(); }

void RemoteRepository::WriteConfig() const {
  nlohmann::json config = {{"name", name_},
                           {"type", "remote"},
                           {"path", path_},
                           {"created_at", created_at_},
                           {"password_hash", GetHashedPassword()}};

  fs::path temp_file = fs::temp_directory_path() / "config.json";
  std::ofstream out(temp_file);
  if (!out) {
    ErrorUtil::ThrowError("Failed to create temporary config file");
  }

  out << config.dump(4);
  out.close();

  if (!UploadFile(temp_file)) {
    ErrorUtil::ThrowError("Failed to upload config to remote");
  }

  fs::remove(temp_file);
}

RemoteRepository RemoteRepository::FromConfigJson(
    const nlohmann::json& config) {
  return RemoteRepository(config.at("path"), config.at("name"),
                          config.value("password_hash", ""),
                          config.at("created_at"));
}

bool RemoteRepository::RemoteDirectoryExists() const {
  ssh_session session = ssh_new();
  sftp_session sftp = nullptr;

  try {
    ssh_options_set(session, SSH_OPTIONS_HOST, host_.c_str());
    ssh_options_set(session, SSH_OPTIONS_USER, user_.c_str());

    if (ssh_connect(session) != SSH_OK ||
        ssh_userauth_publickey_auto(session, nullptr, nullptr) !=
            SSH_AUTH_SUCCESS) {
      ErrorUtil::ThrowError("SSH connection or authentication failed");
    }

    sftp = sftp_new(session);
    if (!sftp || sftp_init(sftp) != SSH_OK) {
      ErrorUtil::ThrowError("SFTP initialization failed");
    }

    sftp_attributes attr = sftp_stat(sftp, remote_dir_.c_str());
    bool exists = attr != nullptr;
    if (attr) sftp_attributes_free(attr);

    sftp_free(sftp);
    ssh_disconnect(session);
    ssh_free(session);

    return exists;
  } catch (...) {
    if (sftp) sftp_free(sftp);
    if (session) {
      ssh_disconnect(session);
      ssh_free(session);
    }
    throw;
  }
}

void RemoteRepository::CreateRemoteDirectory() const {
  ssh_session session = ssh_new();
  sftp_session sftp = nullptr;

  try {
    ssh_options_set(session, SSH_OPTIONS_HOST, host_.c_str());
    ssh_options_set(session, SSH_OPTIONS_USER, user_.c_str());

    if (ssh_connect(session) != SSH_OK ||
        ssh_userauth_publickey_auto(session, nullptr, nullptr) !=
            SSH_AUTH_SUCCESS) {
      ErrorUtil::ThrowError("SSH connection or authentication failed");
    }

    sftp = sftp_new(session);
    if (!sftp || sftp_init(sftp) != SSH_OK) {
      ErrorUtil::ThrowError("SFTP initialization failed");
    }

    if (sftp_mkdir(sftp, remote_dir_.c_str(), S_IRWXU) < 0) {
      ErrorUtil::ThrowError("Remote directory creation failed");
    }
    std::string backup_dir = remote_dir_ + "/backup";
    if (sftp_mkdir(sftp, backup_dir.c_str(), S_IRWXU) < 0) {
      ErrorUtil::ThrowError("Remote directory creation failed");
    }
    std::string chunk_dir = remote_dir_ + "/chunks";
    if (sftp_mkdir(sftp, chunk_dir.c_str(), S_IRWXU) < 0) {
      ErrorUtil::ThrowError("Remote directory creation failed");
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
      if (sftp_mkdir(sftp, (chunk_dir + "/" + i).c_str(), S_IRWXU) < 0) {
        ErrorUtil::ThrowError("Remote directory creation failed");
      }
    }
    sftp_free(sftp);
    ssh_disconnect(session);
    ssh_free(session);
  } catch (...) {
    if (sftp) sftp_free(sftp);
    if (session) {
      ssh_disconnect(session);
      ssh_free(session);
    }
    throw;
  }
}

void RemoteRepository::RemoveRemoteDirectory() const {
  ssh_session session = ssh_new();
  sftp_session sftp = nullptr;

  try {
    ssh_options_set(session, SSH_OPTIONS_HOST, host_.c_str());
    ssh_options_set(session, SSH_OPTIONS_USER, user_.c_str());

    if (ssh_connect(session) != SSH_OK ||
        ssh_userauth_publickey_auto(session, nullptr, nullptr) !=
            SSH_AUTH_SUCCESS) {
      ErrorUtil::ThrowError("SSH connection or authentication failed");
    }

    sftp = sftp_new(session);
    if (!sftp || sftp_init(sftp) != SSH_OK) {
      ErrorUtil::ThrowError("SFTP initialization failed");
    }

    std::function<void(const std::string&)> delete_recursive;
    delete_recursive = [&](const std::string& path) {
      sftp_dir dir = sftp_opendir(sftp, path.c_str());
      if (!dir) {
        sftp_unlink(sftp, path.c_str());
        return;
      }

      while (sftp_attributes attrs = sftp_readdir(sftp, dir)) {
        std::string name = attrs->name;
        if (name != "." && name != "..") {
          std::string full_path = path + "/" + name;
          if (S_ISDIR(attrs->permissions)) {
            delete_recursive(full_path);
          } else {
            sftp_unlink(sftp, full_path.c_str());
          }
        }
        sftp_attributes_free(attrs);
      }

      sftp_closedir(dir);
      sftp_rmdir(sftp, path.c_str());
    };

    delete_recursive(remote_dir_);

    sftp_free(sftp);
    ssh_disconnect(session);
    ssh_free(session);
  } catch (...) {
    if (sftp) sftp_free(sftp);
    if (session) {
      ssh_disconnect(session);
      ssh_free(session);
    }
    throw;
  }
}

bool RemoteRepository::UploadFile(const std::string& local_file,
                                  const std::string& remote_path) const {
  std::string remote_full_path;
  ssh_session session = ssh_new();
  sftp_session sftp = nullptr;

  try {
    if (!fs::exists(local_file)) {
      ErrorUtil::ThrowError("File does not exist: " + local_file);
    }

    fs::path local_fs_path(local_file);
    std::string filename = local_fs_path.filename().string();

    if (remote_path.empty()) {
      remote_full_path = remote_dir_ + "/" + filename;
    } else {
      if (remote_path.back() == '/') {
        remote_full_path = remote_dir_ + "/" + remote_path + filename;
      } else {
        remote_full_path = remote_dir_ + "/" + remote_path + "/" + filename;
      }
    }

    ssh_options_set(session, SSH_OPTIONS_HOST, host_.c_str());
    ssh_options_set(session, SSH_OPTIONS_USER, user_.c_str());

    if (ssh_connect(session) != SSH_OK ||
        ssh_userauth_publickey_auto(session, nullptr, nullptr) !=
            SSH_AUTH_SUCCESS) {
      ErrorUtil::ThrowError("SSH connection or authentication failed");
    }

    sftp = sftp_new(session);
    if (!sftp || sftp_init(sftp) != SSH_OK) {
      ErrorUtil::ThrowError("SFTP initialization failed");
    }

    sftp_file file = sftp_open(sftp, remote_full_path.c_str(),
                               O_WRONLY | O_CREAT | O_TRUNC, S_IRUSR | S_IWUSR);
    if (!file) {
      ErrorUtil::ThrowError("Unable to open remote file for writing: " +
                            remote_full_path);
    }

    std::ifstream input(local_file, std::ios::binary);
    if (!input) {
      sftp_close(file);
      ErrorUtil::ThrowError("Failed to open local file: " + local_file);
    }

    char buffer[16384];
    while (input.read(buffer, sizeof(buffer)) || input.gcount() > 0) {
      if (sftp_write(file, buffer, input.gcount()) < 0) {
        sftp_close(file);
        ErrorUtil::ThrowError("Failed to write to remote file");
      }
    }

    sftp_close(file);
    sftp_free(sftp);
    ssh_disconnect(session);
    ssh_free(session);
    return true;

  } catch (...) {
    // std::cout << ("Failed to upload file to remote path: " + remote_full_path +
    //               " - " + std::string(e.what()));
    if (sftp) sftp_free(sftp);
    if (session) {
      ssh_disconnect(session);
      ssh_free(session);
    }
    ErrorUtil::ThrowNested("Cannot upload file to remote path: " +
                           remote_full_path);
  }
  return false;
}

bool RemoteRepository::UploadDirectory(const std::string& local_dir,
                                       const std::string& remote_path) const {
  std::string remote_path_ = remote_dir_ + "/" + remote_path;

  ssh_session session = ssh_new();
  sftp_session sftp = nullptr;

  try {
    fs::path local_path(local_dir);
    if (!fs::is_directory(local_path)) {
      ErrorUtil::ThrowError("Local directory does not exist: " + local_dir);
    }

    if (!session) {
      ErrorUtil::ThrowError("Failed to create SSH session");
    }

    ssh_options_set(session, SSH_OPTIONS_HOST, host_.c_str());
    ssh_options_set(session, SSH_OPTIONS_USER, user_.c_str());

    if (ssh_connect(session) != SSH_OK ||
        ssh_userauth_publickey_auto(session, nullptr, nullptr) !=
            SSH_AUTH_SUCCESS) {
      ssh_free(session);
      ErrorUtil::ThrowError("SSH connection or authentication failed");
    }

    sftp = sftp_new(session);
    if (!sftp || sftp_init(sftp) != SSH_OK) {
      ssh_disconnect(session);
      ssh_free(session);
      ErrorUtil::ThrowError("Failed to initialize SFTP session");
    }

    std::function<void(const fs::path&, const std::string&)> upload_recursive;
    upload_recursive = [&](const fs::path& path,
                           const std::string& remote_path) {
      if (fs::is_directory(path)) {
        std::string dir_name = path.filename().string();
        std::string remote_subdir = remote_path + "/" + dir_name;

        if (sftp_mkdir(sftp, remote_subdir.c_str(), S_IRWXU) < 0 &&
            sftp_get_error(sftp) != SSH_FX_FAILURE) {
          ErrorUtil::ThrowError("Failed to create remote directory: " +
                                remote_subdir);
        }

        for (const auto& entry : fs::directory_iterator(path)) {
          upload_recursive(entry.path(), remote_subdir);
        }
      } else if (fs::is_regular_file(path)) {
        std::string file_name = path.filename().string();
        const std::string remote_file = remote_path + "/" + file_name;

        std::ifstream infile(path, std::ios::binary);
        if (!infile) return;

        const int max_retries = 3;
        int attempt = 0;

        while (attempt < max_retries) {
          sftp_file file =
              sftp_open(sftp, remote_file.c_str(), O_WRONLY | O_CREAT | O_TRUNC,
                        S_IRUSR | S_IWUSR);
          if (!file) {
            attempt++;
            continue;
          }

          char buffer[16384];
          while (infile) {
            infile.read(buffer, sizeof(buffer));
            std::streamsize bytes = infile.gcount();
            if (bytes > 0 && sftp_write(file, buffer, bytes) < 0) {
              sftp_close(file);
              attempt++;
              break;
            }
          }
          sftp_close(file);
          break;
        }

        if (attempt == max_retries) {
          ErrorUtil::ThrowError("Failed to upload file after retries: " +
                                path.string());
        }
      }
    };

    for (const auto& entry : fs::directory_iterator(local_dir)) {
      upload_recursive(entry.path(), remote_path_);
    }

    sftp_free(sftp);
    ssh_disconnect(session);
    ssh_free(session);
    return true;

  } catch (...) {
    if (sftp) sftp_free(sftp);
    if (session) {
      ssh_disconnect(session);
      ssh_free(session);
    }
    ErrorUtil::ThrowNested("Cannot upload directory to remote path: " +
                           (remote_path_));
  }
  return false;
}

bool RemoteRepository::DownloadFile(const std::string& remote_file,
                                    const std::string& local_path) const {
  std::string remote_full_path = remote_dir_ + "/" + remote_file;

  ssh_session session = ssh_new();
  sftp_session sftp = nullptr;

  try {
    fs::path remote_fs_path(remote_file);
    std::string filename = remote_fs_path.filename().string();

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

    ssh_options_set(session, SSH_OPTIONS_HOST, host_.c_str());
    ssh_options_set(session, SSH_OPTIONS_USER, user_.c_str());

    if (ssh_connect(session) != SSH_OK ||
        ssh_userauth_publickey_auto(session, nullptr, nullptr) !=
            SSH_AUTH_SUCCESS) {
      ErrorUtil::ThrowError("SSH connection or authentication failed");
    }

    sftp = sftp_new(session);
    if (!sftp || sftp_init(sftp) != SSH_OK) {
      ErrorUtil::ThrowError("SFTP initialization failed");
    }

    sftp_file file = sftp_open(sftp, remote_full_path.c_str(), O_RDONLY, 0);
    if (!file) {
      ErrorUtil::ThrowError("Unable to open remote file for reading: " +
                            remote_full_path);
    }

    std::ofstream output(local_full_path, std::ios::binary);
    if (!output) {
      sftp_close(file);
      ErrorUtil::ThrowError("Failed to open local file for writing: " +
                            local_full_path);
    }

    char buffer[16384];
    int nbytes;
    while ((nbytes = sftp_read(file, buffer, sizeof(buffer))) > 0) {
      output.write(buffer, nbytes);
    }

    if (nbytes < 0) {
      sftp_close(file);
      ErrorUtil::ThrowError("Error reading from remote file: " +
                            remote_full_path);
    }

    sftp_close(file);
    sftp_free(sftp);
    ssh_disconnect(session);
    ssh_free(session);
    return true;

  } catch (...) {
    if (sftp) sftp_free(sftp);
    if (session) {
      ssh_disconnect(session);
      ssh_free(session);
    }
    ErrorUtil::ThrowNested("Cannot download file from remote path: " +
                           remote_full_path);
  }
  return false;
}

bool RemoteRepository::DownloadDirectory(const std::string& remote_dir,
                                         const std::string& local_path) const {
  std::string remote_root = remote_dir_ + "/" + remote_dir;

  ssh_session session = ssh_new();
  sftp_session sftp = nullptr;

  try {
    if (!fs::exists(local_path)) {
      fs::create_directories(local_path);
    }

    ssh_options_set(session, SSH_OPTIONS_HOST, host_.c_str());
    ssh_options_set(session, SSH_OPTIONS_USER, user_.c_str());

    if (ssh_connect(session) != SSH_OK ||
        ssh_userauth_publickey_auto(session, nullptr, nullptr) !=
            SSH_AUTH_SUCCESS) {
      ErrorUtil::ThrowError("SSH connection or authentication failed");
    }

    sftp = sftp_new(session);
    if (!sftp || sftp_init(sftp) != SSH_OK) {
      ErrorUtil::ThrowError("SFTP initialization failed");
    }

    std::function<void(const std::string&, const fs::path&)> download_recursive;
    download_recursive = [&](const std::string& remote_subpath,
                             const fs::path& local_subdir) {
      sftp_dir dir = sftp_opendir(sftp, remote_subpath.c_str());
      if (!dir) {
        ErrorUtil::ThrowError("Cannot open remote directory: " +
                              remote_subpath);
      }

      fs::create_directories(local_subdir);

      while (sftp_attributes attr = sftp_readdir(sftp, dir)) {
        std::string name = attr->name;
        if (name == "." || name == "..") {
          sftp_attributes_free(attr);
          continue;
        }

        std::string full_remote = remote_subpath + "/" + name;
        fs::path full_local = local_subdir / name;

        if (S_ISDIR(attr->permissions)) {
          download_recursive(full_remote, full_local);
        } else if (S_ISREG(attr->permissions)) {
          sftp_file file = sftp_open(sftp, full_remote.c_str(), O_RDONLY, 0);
          if (!file) {
            sftp_attributes_free(attr);
            continue;
          }

          std::ofstream output(full_local, std::ios::binary);
          if (!output) {
            sftp_close(file);
            sftp_attributes_free(attr);
            continue;
          }

          char buffer[16384];
          int nbytes;
          while ((nbytes = sftp_read(file, buffer, sizeof(buffer))) > 0) {
            output.write(buffer, nbytes);
          }

          sftp_close(file);
        }

        sftp_attributes_free(attr);
      }

      sftp_closedir(dir);
    };

    download_recursive(remote_root, fs::path(local_path));

    sftp_free(sftp);
    ssh_disconnect(session);
    ssh_free(session);
    return true;

  } catch (...) {
    if (sftp) sftp_free(sftp);
    if (session) {
      ssh_disconnect(session);
      ssh_free(session);
    }
    ErrorUtil::ThrowNested("Cannot download directory from remote path: " +
                           remote_root);
  }
  return false;
}
