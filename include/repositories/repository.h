#ifndef REPOSITORY_H_
#define REPOSITORY_H_

#include <string>

class Repository {
 public:
  Repository() = default;
  virtual ~Repository() {}

  virtual bool Exists() const = 0;
  virtual void Initialize() = 0;
  // virtual void Delete() = 0;
  
  virtual std::string GetName() const = 0;
  // virtual std::string GetPath() const = 0;
};

#endif  // REPOSITORY_H_
