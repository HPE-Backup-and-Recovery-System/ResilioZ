#ifndef RESTORE_SYSTEM_H
#define RESTORE_SYSTEM_H

#include "systems/system.h"
#include "services/all.h"

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

  RepositoryService* repo_service_;
};

#endif  // RESTORE_SYSTEM_H
