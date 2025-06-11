#ifndef CLI_USERIO_H_
#define CLI_USERIO_H_

#include "userio.h"

class CLIUserIO : public UserIO {
 public:
  void Print(const std::string& message) override;
};

#endif  // CLI_USERIO_H_