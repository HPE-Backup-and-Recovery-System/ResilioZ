#ifndef REPOSITORY_H_
#define REPOSITORY_H_

#include <string>

class Repository {
 public:
  Repository() = default;
  virtual ~Repository() {}

  virtual bool Exists() const = 0;
  virtual void Initialize() = 0;
  virtual void Delete() = 0;

  virtual void WriteConfigToRepo() const = 0;

  std::string GetName() const;
  std::string GetPath() const;

  std::string GetHashedPassword() const;
  static std::string GetHashedPassword(const std::string& password);

 protected:
  std::string name_;
  std::string path_;
  std::string password_;
  std::string created_at_;
};

#endif  // REPOSITORY_H_
