#ifndef REPOSITORY_H_
#define REPOSITORY_H_

#include <string>

enum class RepositoryType { LOCAL, NFS, REMOTE };

class Repository {
 public:
  Repository() = default;
  virtual ~Repository() = default;

  virtual bool Exists() const = 0;
  virtual void Initialize() = 0;
  virtual void Delete() = 0;

  virtual void WriteConfig() const = 0;

  std::string GetName() const;
  std::string GetPath() const;
  std::string GetFullPath() const;
  RepositoryType GetType() const;
  std::string GetPassword() const;

  std::string GetHashedPassword() const;
  std::string GetRepositoryInfoString() const;

  static std::string GetRepositoryInfoString(const std::string &name,
                                             const std::string &type,
                                             const std::string &path);
  static std::string GetRepositoryInfoString(const std::string &name,
                                             const RepositoryType &type,
                                             const std::string &path);

  static std::string GetHashedPassword(const std::string &password);
  static std::string GetResolvedPath(const std::string &path);

  static std::string GetFormattedTypeString(const std::string &type,
                                            bool upper = true);
  static std::string GetFormattedTypeString(const RepositoryType &type,
                                            bool upper = true);

  virtual bool UploadFile(const std::string &source_file,
                          const std::string &destination_path) const = 0;
  virtual bool UploadDirectory(const std::string &source_dir,
                               const std::string &destination_path) const = 0;
  virtual bool DownloadFile(const std::string &source_file,
                            const std::string &destination_path) const = 0;
  virtual bool DownloadDirectory(const std::string &source_dir,
                                 const std::string &destination_path) const = 0;

 protected:
  std::string name_;
  std::string path_;
  std::string password_;
  std::string created_at_;
  RepositoryType type_;
};

#endif  // REPOSITORY_H_
