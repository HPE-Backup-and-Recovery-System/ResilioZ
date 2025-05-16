#ifndef REPO_SERVICE_H_
#define REPO_SERVICE_H_

#include "repositories/repository.h"
#include "service.h"

class RepositoryService : public Service {
 public:
  RepositoryService();
  ~RepositoryService() {}

  void Run() override;
  void Log() override;

 private:
  void ShowMainMenu();
  void HandleNewRepository();
  void HandleExistingRepository();
  void HandleLocalRepository();
  void HandleRemoteRepository();

  Repository* repository_;
};

#endif  // REPO_SERVICE_H_
