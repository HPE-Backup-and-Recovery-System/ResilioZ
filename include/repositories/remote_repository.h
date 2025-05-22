#ifndef REMOTE_REPOSITORY_H_
#define REMOTE_REPOSITORY_H_

#include <nlohmann/json.hpp>

#include "repositories/repository.h"

class RemoteRepository : public Repository {
 public:
  RemoteRepository();
  RemoteRepository(const std::string& path, const std::string& name,
                   const std::string& password, const std::string& created_at);
  ~RemoteRepository() {}

  bool Exists() const override;
  void Initialize() override;
  void Delete() override;

  void WriteConfig() const override;
  static RemoteRepository FromConfigJson(const nlohmann::json& config);

 private:
  std::string user_;
  std::string host_;
  std::string remote_dir_;

  void ParseSFTPPath(const std::string& sftp_path);
  bool UploadFile(const std::string& local_file,
                  const std::string& remote_path) const;
  bool RemoteDirectoryExists() const;
  void CreateRemoteDirectory() const;
  void RemoveRemoteDirectory() const;
};

#endif  // REMOTE_REPOSITORY_H_
