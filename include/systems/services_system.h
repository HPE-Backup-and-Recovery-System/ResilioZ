#ifndef SERVICES_SYSTEM_H_
#define SERVICES_SYSTEM_H_

#include <chrono>
#include <string>
#include <vector>

#include "services/service.h"
#include "systems/system.h"

class ServicesSystem : public System {
 public:
  ServicesSystem();
  ~ServicesSystem() {};

  void Start() override;
  void Shutdown() override;
  void Log() override;

 private:
  std::vector<Service*> services_;
};

#endif  // SERVICES_SYSTEM_H_
