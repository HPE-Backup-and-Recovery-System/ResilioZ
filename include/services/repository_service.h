#ifndef REPOSITORY_SERVICE_H_
#define REPOSITORY_SERVICE_H_

#include "repositories/all.h"
#include "service.h"
#include "utils/repodata_manager.h"

class RepositoryService : public Service {
 public:
  RepositoryService();
  ~RepositoryService();

  void Run() override;
  void Log() override;

  bool CreateNewRepository(bool loop = false);
  void ListRepositories();
  std::vector<RepoEntry> GetAllRepositories() const;
  Repository* SelectExistingRepository();

  Repository* GetRepository();
  void SetRepository(Repository* new_repo);

 private:
  void ShowMainMenu();
  void InitLocalRepositoryFromPrompt();
  void InitNFSRepositoryFromPrompt();
  void InitRemoteRepositoryFromPrompt();
  void DeleteRepository();

  Repository* repository_;
  RepodataManager* repodata_mgr;
};

#endif  // REPOSITORY_SERVICE_H_
