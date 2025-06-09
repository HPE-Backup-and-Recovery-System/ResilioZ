#ifndef SYSTEM_H_
#define SYSTEM_H_

#include <string>

class System {
 public:
  virtual void Start() = 0;
  virtual void Shutdown() = 0;
  virtual void Log() = 0;

  virtual ~System() = default;
};

#endif  // SYSTEM_H_