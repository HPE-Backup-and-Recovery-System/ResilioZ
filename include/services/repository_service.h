#ifndef REPO_SERVICE_H_
#define REPO_SERVICE_H_

#include "repositories/all.h"
#include "service.h"
#include "utils/repodata_manager.h"

class RepositoryService : public Service {
 public:
  RepositoryService();
  ~RepositoryService() {};

  void Run() override;
  void Log() override;

  void CreateNewRepository(bool loop = false);
  void ListRepositories();
  Repository* FetchExistingRepository();

  Repository* GetRepository();
  void SetRepository(Repository* new_repo);

 private:
  void ShowMainMenu();
  void InitLocalRepositoryFromPrompt();
  void InitNFSRepositoryFromPrompt();
  void InitRemoteRepositoryFromPrompt();
  void DeleteRepository();

  Repository* repository_;
  RepodataManager repodata_;
};

#endif  // REPO_SERVICE_H_
