#ifndef RESTORE_SYSTEM_H
#define RESTORE_SYSTEM_H

#include "systems/system.h"

class RestoreSystem : public System {
 public:
  RestoreSystem();
  ~RestoreSystem() {};

  void Start() override;
  void Shutdown() override;
  void Log() override;
};

#endif  // RESTORE_SYSTEM_H
