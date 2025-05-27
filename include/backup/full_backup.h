#ifndef BACKUP_FULL_BACKUP_H_
#define BACKUP_FULL_BACKUP_H_

#include "backup/backup.h"
#include <string>
#include <filesystem>



class FullBackup : public Backup {
 public:
  FullBackup() = default;
  ~FullBackup() override = default;

  void Execute(const std::string& source_directory_path,
               const std::string& destination_path) override;
};

 // namespace backup

#endif  // BACKUP_FULL_BACKUP_H_
