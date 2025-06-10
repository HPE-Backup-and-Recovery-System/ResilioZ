#ifndef BACKUP_SYSTEM_H
#define BACKUP_SYSTEM_H

#include "repositories/repository.h"
#include "services/all.h"
#include "systems/system.h"

class BackupSystem : public System {
 public:
  BackupSystem();
  ~BackupSystem();

  void Run() override;
  void Log() override;

 private:
  void CreateBackup();
  void ListBackups();
  void CompareBackups();
  void ScheduleBackup();

  Repository* repository_;
  RepositoryService* repo_service_;
  SchedulerService* scheduler_service_;
};

#endif  // BACKUP_SYSTEM_H
