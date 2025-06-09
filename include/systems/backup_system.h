#ifndef BACKUP_SYSTEM_H
#define BACKUP_SYSTEM_H

#include "systems/system.h"

class BackupSystem : public System {
 public:
  BackupSystem();
  ~BackupSystem() {};

  void Start() override;
  void Shutdown() override;
  void Log() override;
};

#endif  // BACKUP_SYSTEM_H
