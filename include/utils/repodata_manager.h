#ifndef REPODATA_MANAGER_H_
#define REPODATA_MANAGER_H_

#include <nlohmann/json.hpp>
#include <string>
#include <vector>

#include "repositories/repository.h"

struct RepoEntry {
  std::string name;
  std::string path;
  std::string type;
  std::string password_hash = "";
  std::string created_at;
};

class RepodataManager {
 public:
  RepodataManager();
  bool Load();
  bool Save();

  void AddEntry(const RepoEntry& entry);
  bool DeleteEntry(const std::string& name, const std::string& path);
  std::optional<RepoEntry> GetEntry(const std::string& name,
                                    const std::string& path) const;
  std::vector<RepoEntry> GetAll() const;

 private:
  std::vector<RepoEntry> entries_;
  std::string data_file_;

  void EnsureDataFileExists();
};

#endif  // REPODATA_MANAGER_H_
