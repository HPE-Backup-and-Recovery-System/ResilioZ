#ifndef REMOTE_REPOSITORY_H_
#define REMOTE_REPOSITORY_H_

#include "repository.h"

class RemoteRepository : public Repository {
 public:
  RemoteRepository();
  RemoteRepository(const std::string& ip, const std::string& username,
                   const std::string& password, const std::string& name,
                   const std::string& path, const std::string& created_at);
  ~RemoteRepository() {}

  bool Exists() const override;
  void Initialize() override;
  // void Delete() override;

  std::string GetName() const override;
  // std::string GetPath() const override;

 private:
  std::string ip_;
  std::string username_;
  std::string password_;
  std::string name_;
  std::string path_;
  std::string created_at_;
};

#endif  // REMOTE_REPOSITORY_H_
