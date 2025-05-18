#ifndef LOCAL_REPOSITORY_H_
#define LOCAL_REPOSITORY_H_

#include "repository.h"

class LocalRepository : public Repository {
 public:
  LocalRepository();
  explicit LocalRepository(const std::string& path, const std::string& name, const std::string& created_at);
  ~LocalRepository() {}

  bool Exists() const override;
  void Initialize() override;
  // void Delete() override;

  std::string GetName() const override;
  // std::string GetPath() const override;
  
 private:
  std::string path_;
  std::string name_;
  std::string created_at_;
};

#endif  // LOCAL_REPOSITORY_H_
