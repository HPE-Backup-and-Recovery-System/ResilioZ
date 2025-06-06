#ifndef BACKUP_DIFFERENTIAL_BACKUP_H_
#define BACKUP_DIFFERENTIAL_BACKUP_H_

#include "backup/backup.h"
#include <string>
#include <filesystem>
#include <archive.h>
#include <archive_entry.h>
#include <nlohmann/json.hpp>

namespace backup {
     namespace fs = std::filesystem;
    using json = nlohmann::json;
class DifferentialBackup : public Backup {
 public:
  DifferentialBackup() = default;
  ~DifferentialBackup() override = default;

  void PerformBackup(const std::string& config_path,
               const std::string& destination_path) override;
    private:
    void AddModifiedFilesToArchive(struct archive* archive_ptr,
                                   const fs::path& source_dir,
                                   const json& metadata);
    nlohmann::json LoadLatestFullMetadata(const std::string& log_file);
    bool IsFileModified(const fs::path& file_path, const fs::path& source_dir,
        const json& metadata);
};

}   // namespace backup

#endif  // BACKUP_DIFFERENTIAL_BACKUP_H_
