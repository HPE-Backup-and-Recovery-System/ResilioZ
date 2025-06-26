#ifndef NFS_REPOSITORY_H_
#define NFS_REPOSITORY_H_

#include <nlohmann/json.hpp>

#include "repository.h"

class NFSRepository : public Repository {
 public:
  NFSRepository();
  NFSRepository(const std::string& nfs_path, const std::string& name,
                const std::string& password, const std::string& created_at);
  ~NFSRepository() {}

  bool Exists() const override;
  void Initialize() override;
  void Delete() override;

  void WriteConfig() const override;
  static NFSRepository FromConfigJson(const nlohmann::json& config);

  bool UploadFile(const std::string& local_file,
                  const std::string& remote_path = "") const override;

  bool UploadDirectory(const std::string& local_dir,
                      const std::string& remote_path = "") const override;

  // Add methods for restore system
  std::vector<std::string> ListFiles(const std::string& remote_dir) const;
  bool DownloadFile(const std::string& remote_file, const std::string& local_file) const override;
  bool DownloadDirectory(const std::string& remote_dir, const std::string& local_path) const override;

 private:
  void ParseNfsPath(const std::string& nfs_path);
  bool NFSMountExists() const;
  void CreateRemoteDirectory() const;
  void RemoveRemoteDirectory() const;

  std::string server_ip_;
  std::string server_backup_path_;
};

#endif  // NFS_REPOSITORY_H_