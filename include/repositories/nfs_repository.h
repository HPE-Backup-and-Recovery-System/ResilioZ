#ifndef NFS_REPOSITORY_H_
#define NFS_REPOSITORY_H_

#include <nlohmann/json.hpp>

#include "repository.h"

class NFSRepository : public Repository {
 public:
  NFSRepository();
  NFSRepository(const std::string& nfs_mount_path, const std::string& name,
                const std::string& password, const std::string& created_at);
  NFSRepository(const std::string& server_ip,
                const std::string& server_backup_path, const std::string& name,
                const std::string& password, const std::string& created_at);
  ~NFSRepository() {}

  bool Exists() const override;
  void Initialize() override;
  void Delete() override;

  void WriteConfig() const override;
  static NFSRepository FromConfigJson(const nlohmann::json& config);

 private:
  bool UploadFile(const std::string& local_file,
                  const std::string& remote_path) const;
  bool NFSMountExists() const;
  void EnsureNFSMounted() const;
  void CreateRemoteDirectory() const;
  void RemoveRemoteDirectory() const;

  std::string server_ip_;
  std::string server_backup_path_;
};

#endif  // NFS_REPOSITORY_H_