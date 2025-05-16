#ifndef REMOTE_REPOSITORY_H_
#define REMOTE_REPOSITORY_H_

#include "repository.h"

class RemoteRepository : public Repository {
 public:
  RemoteRepository();
  RemoteRepository(const std::string& ip, const std::string& username,
                   const std::string& password, const std::string& name);
  ~RemoteRepository() {}

  bool Exists() const override;
  void Initialize() override;
  std::string GetName() const override;

 private:
  std::string ip_;
  std::string username_;
  std::string password_;
  std::string name_;
};

#endif  // REMOTE_REPOSITORY_H_
