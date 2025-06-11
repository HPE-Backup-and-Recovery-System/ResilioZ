#ifndef USERIO_H_
#define USERIO_H_

#include <string>
#include <vector>

class UserIO {
 public:
  virtual ~UserIO() = default;

  virtual void Print(const std::string& message) = 0;
};

#endif  // USERIO_H_
