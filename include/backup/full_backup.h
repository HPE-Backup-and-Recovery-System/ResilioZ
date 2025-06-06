#ifndef BACKUP_FULL_BACKUP_H_
#define BACKUP_FULL_BACKUP_H_

#include "backup/backup.h"
#include <string>
#include <filesystem>
#include <archive.h>
#include <archive_entry.h>


namespace backup {
namespace fs = std::filesystem;

class FullBackup : public Backup {
 public:
  FullBackup() = default;
  ~FullBackup() override = default;

  void PerformBackup(const std::string& source_directory_path,
               const std::string& destination_path) override;
    private:
    void AddDirectoryToArchive(struct archive* archive_ptr,
        const fs::path& dir_path,
        const fs::path& base_path);
};


}  // namespace backup

#endif  // BACKUP_FULL_BACKUP_H_
