#ifndef LOCAL_REPOSITORY_H_
#define LOCAL_REPOSITORY_H_

#include "repository.h"

class LocalRepository : public Repository {
 public:
  LocalRepository();
  explicit LocalRepository(const std::string& path, const std::string& name);
  ~LocalRepository() {}

  bool Exists() const override;
  void Initialize() override;
  std::string GetName() const override;

 private:
  std::string path_;
  std::string name_;
};

#endif  // LOCAL_REPOSITORY_H_
