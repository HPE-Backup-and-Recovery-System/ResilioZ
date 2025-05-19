#ifndef LOCAL_REPOSITORY_H_
#define LOCAL_REPOSITORY_H_

#include <nlohmann/json.hpp>

#include "repository.h"

class LocalRepository : public Repository {
 public:
  LocalRepository();
  LocalRepository(const std::string& path, const std::string& name,
                  const std::string& password, const std::string& created_at);
  ~LocalRepository() {}

  bool Exists() const override;
  void Initialize() override;
  void Delete() override;

  void WriteConfigToRepo() const override;
  static LocalRepository FromConfigJson(const nlohmann::json& config);

 private:
  std::string GetFullPath() const;
};

#endif  // LOCAL_REPOSITORY_H_
