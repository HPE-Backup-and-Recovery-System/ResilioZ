#include "remote_repository.h"

#include <fcntl.h>
#include <libssh/libssh.h>
#include <libssh/sftp.h>
#include <sys/stat.h>

#include <filesystem>
#include <fstream>
#include <iostream>
#include <sstream>

RemoteRepository::RemoteRepository() {}

RemoteRepository::RemoteRepository(const std::string& sftp_path,
                                   const std::string& name,
                                   const std::string& password,
                                   const std::string& created_at) {
  ParseSFTPPath(sftp_path);
  name_ = name;
  path_ = sftp_path;
  password_ = password;
  created_at_ = created_at;
}

RemoteRepository::~RemoteRepository() {}

void RemoteRepository::ParseSFTPPath(const std::string& sftp_path) {
  auto at_pos = sftp_path.find('@');
  auto colon_pos = sftp_path.find(':');

  if (at_pos == std::string::npos || colon_pos == std::string::npos ||
      colon_pos < at_pos) {
    throw std::runtime_error(
        "Invalid SFTP Path Format! Expected: user@host:/path...");
  }

  user_ = sftp_path.substr(0, at_pos);
  host_ = sftp_path.substr(at_pos + 1, colon_pos - at_pos - 1);
  remote_dir_ = sftp_path.substr(colon_pos + 1);
  if (remote_dir_.back() == '/')
    remote_dir_.pop_back();  // Remove Trailing Slash
}

bool RemoteRepository::Exists() const { return RemoteDirectoryExists(); }

void RemoteRepository::Initialize() {
  CreateRemoteDirectory();
  WriteConfigToRepo();
}

void RemoteRepository::Delete() { RemoveRemoteDirectory(); }

void RemoteRepository::WriteConfigToRepo() const {
  nlohmann::json config = {{"name", name_},
                           {"type", "remote"},
                           {"path", path_},
                           {"created_at", created_at_},
                           {"password_hash", GetHashedPassword()}};

  std::string temp_file = "/tmp/repo_config_temp.json";
  std::ofstream out(temp_file);
  out << config.dump(4);
  out.close();

  UploadFile(temp_file, remote_dir_ + "/config.json");

  std::filesystem::remove(temp_file);
}

RemoteRepository RemoteRepository::FromConfigJson(
    const nlohmann::json& config) {
  return RemoteRepository(config.at("path"), config.at("name"),
                          config.value("password_hash", ""),
                          config.at("created_at"));
}

bool RemoteRepository::RemoteDirectoryExists() const {
  ssh_session session = ssh_new();
  ssh_options_set(session, SSH_OPTIONS_HOST, host_.c_str());
  ssh_options_set(session, SSH_OPTIONS_USER, user_.c_str());

  if (ssh_connect(session) != SSH_OK ||
      ssh_userauth_publickey_auto(session, nullptr, nullptr) !=
          SSH_AUTH_SUCCESS) {
    ssh_free(session);
    return false;
  }

  sftp_session sftp = sftp_new(session);
  if (!sftp || sftp_init(sftp) != SSH_OK) {
    sftp_free(sftp);
    ssh_disconnect(session);
    ssh_free(session);
    return false;
  }

  sftp_attributes attr = sftp_stat(sftp, remote_dir_.c_str());
  bool exists = attr != nullptr;
  if (attr) sftp_attributes_free(attr);

  sftp_free(sftp);
  ssh_disconnect(session);
  ssh_free(session);

  return exists;
}

void RemoteRepository::CreateRemoteDirectory() const {
  ssh_session session = ssh_new();
  ssh_options_set(session, SSH_OPTIONS_HOST, host_.c_str());
  ssh_options_set(session, SSH_OPTIONS_USER, user_.c_str());

  if (ssh_connect(session) != SSH_OK ||
      ssh_userauth_publickey_auto(session, nullptr, nullptr) !=
          SSH_AUTH_SUCCESS) {
    throw std::runtime_error("SSH Connection or Authentication Failed...");
  }

  sftp_session sftp = sftp_new(session);
  if (!sftp || sftp_init(sftp) != SSH_OK) {
    throw std::runtime_error("SFTP Initialization Failed...");
  }

  if (sftp_mkdir(sftp, remote_dir_.c_str(), S_IRWXU) < 0) {
    sftp_free(sftp);
    ssh_disconnect(session);
    ssh_free(session);
    throw std::runtime_error("Remote Directory Creation Failure!");
  }

  sftp_free(sftp);
  ssh_disconnect(session);
  ssh_free(session);
}

void RemoteRepository::RemoveRemoteDirectory() const {
  ssh_session session = ssh_new();
  ssh_options_set(session, SSH_OPTIONS_HOST, host_.c_str());
  ssh_options_set(session, SSH_OPTIONS_USER, user_.c_str());

  if (ssh_connect(session) != SSH_OK ||
      ssh_userauth_publickey_auto(session, nullptr, nullptr) !=
          SSH_AUTH_SUCCESS) {
    throw std::runtime_error("SSH Connection or Authentication Failed...");
  }

  sftp_session sftp = sftp_new(session);
  if (!sftp || sftp_init(sftp) != SSH_OK) {
    throw std::runtime_error("SFTP initialization failed.");
  }

  sftp_unlink(sftp, (remote_dir_ + "/config.json").c_str());
  sftp_rmdir(sftp, remote_dir_.c_str());

  sftp_free(sftp);
  ssh_disconnect(session);
  ssh_free(session);
}

bool RemoteRepository::UploadFile(const std::string& local_file,
                                  const std::string& remote_path) const {
  ssh_session session = ssh_new();
  ssh_options_set(session, SSH_OPTIONS_HOST, host_.c_str());
  ssh_options_set(session, SSH_OPTIONS_USER, user_.c_str());

  if (ssh_connect(session) != SSH_OK ||
      ssh_userauth_publickey_auto(session, nullptr, nullptr) !=
          SSH_AUTH_SUCCESS) {
    return false;
  }

  sftp_session sftp = sftp_new(session);
  if (!sftp || sftp_init(sftp) != SSH_OK) {
    return false;
  }

  sftp_file file = sftp_open(sftp, remote_path.c_str(),
                             O_WRONLY | O_CREAT | O_TRUNC, S_IRUSR | S_IWUSR);
  if (!file) {
    return false;
  }

  std::ifstream input(local_file, std::ios::binary);
  std::ostringstream buffer;
  buffer << input.rdbuf();
  std::string content = buffer.str();

  if (sftp_write(file, content.c_str(), content.size()) < 0) {
    sftp_close(file);
    return false;
  }

  sftp_close(file);
  sftp_free(sftp);
  ssh_disconnect(session);
  ssh_free(session);
  return true;
}
