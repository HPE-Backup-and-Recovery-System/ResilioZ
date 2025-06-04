#ifndef REPO_SERVICE_H_
#define REPO_SERVICE_H_

#include "repositories/repository.h"
#include "utils/repodata_manager.h"
#include "service.h"

class RepositoryService : public Service {
 public:
  RepositoryService();
  ~RepositoryService() {}

  void Run() override;
  void Log() override;

 private:
  void ShowMainMenu();
  void CreateNewRepository();
  void InitLocalRepositoryFromPrompt();
  void InitNFSRepositoryFromPrompt();
  void InitRemoteRepositoryFromPrompt();
  void ListRepositories();
  void UseExistingRepository();
  void DeleteRepository();

  Repository* repository_;
  RepodataManager repodata_;
};

#endif  // REPO_SERVICE_H_
