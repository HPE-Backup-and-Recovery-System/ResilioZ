#ifndef RESTORE_SYSTEM_H
#define RESTORE_SYSTEM_H

#include "services/all.h"
#include "systems/system.h"

class RestoreSystem : public System {
 public:
  RestoreSystem();
  ~RestoreSystem();

  void Run() override;
  void Log() override;

 private:
  void RestoreFromBackup();
  void ListBackups();
  void CompareBackups();
  // void ResumeFailedRestore();

  RepositoryService* repo_service_;
  Repository* repository_;
};

#endif  // RESTORE_SYSTEM_H
