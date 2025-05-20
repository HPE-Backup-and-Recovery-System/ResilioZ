#ifndef BACKUP_SERVICE_H_
#define BACKUP_SERVICE_H_

#include "service.h"



class BackupService : public Service {
 public:
  BackupService();
  ~BackupService() override;

  void Run() override;
  void Log() override;

 private:
  void ShowMainMenu();
  void HandleFullBackup();
  void HandleDifferentialBackup();
};



#endif  // BACKUP_SERVICE_H_
