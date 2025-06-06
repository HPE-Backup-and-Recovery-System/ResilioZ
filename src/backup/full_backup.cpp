#include "backup/full_backup.h"

#include <archive.h>
#include <archive_entry.h>
#include <ctime>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <nlohmann/json.hpp>
#include <vector>
#include "utils/backup_logger.h"
#include "utils/metadata_logger.h"
#include  "utils/backup_utils.h"


namespace backup {
  namespace fs = std::filesystem;
  using json = nlohmann::json;
  using bu = utils::BackupUtils;


// Recursively adds files/directories to the archive
void FullBackup::AddDirectoryToArchive(struct archive* archive_ptr,
                           const fs::path& dir_path,
                           const fs::path& base_path) {
  for (const auto& entry : fs::recursive_directory_iterator(dir_path)) {
    const auto relative_path = fs::relative(entry.path(), base_path);
    struct archive_entry* archive_entry = archive_entry_new();
    archive_entry_set_pathname(archive_entry, relative_path.string().c_str());

    if (fs::is_directory(entry.path())) {
      archive_entry_set_filetype(archive_entry, AE_IFDIR);
      archive_entry_set_perm(archive_entry, 0755);
      archive_write_header(archive_ptr, archive_entry);
    } else if (fs::is_regular_file(entry.path())) {
      archive_entry_set_filetype(archive_entry, AE_IFREG);
      archive_entry_set_perm(archive_entry, 0644);
      archive_entry_set_size(archive_entry, fs::file_size(entry.path()));

      archive_write_header(archive_ptr, archive_entry);

      std::ifstream ifs(entry.path(), std::ios::binary);
      std::vector<char> buffer(8192);
      while (ifs.read(buffer.data(), buffer.size()) || ifs.gcount() > 0) {
        archive_write_data(archive_ptr, buffer.data(), ifs.gcount());
      }
    }

    archive_entry_free(archive_entry);
  }
}

void FullBackup::PerformBackup(const std::string& config_path,
                               const std::string& destination_path) {
  try {
    std::string source_path = bu::GetSourcePathFromConfig(config_path);
    std::string timestamp = bu::GetCurrentTimestamp();
    fs::path backups_dir = fs::path(destination_path) / "backups";
    if (!fs::exists(backups_dir)) {
      fs::create_directories(backups_dir);
    }
    fs::path individual_backup_dir = backups_dir / ("full_backup_" + timestamp);
    fs::create_directories(individual_backup_dir);
    std::string archive_file = individual_backup_dir.string() +
                               "/full_backup_" + timestamp + ".tar.zst";
    std::string metadata_file = individual_backup_dir.string() +
                                "/full_backup_" + timestamp + "_metadata.json";
    std::string log_file = backups_dir.string() + "/full_backups.log";

    // Setup libarchive
    struct archive* archive_ptr = archive_write_new();

    if (archive_write_add_filter_zstd(archive_ptr) != ARCHIVE_OK) {
      throw std::runtime_error("Failed to set Zstd compression: " +
                               std::string(archive_error_string(archive_ptr)));
    }

    if (archive_write_set_format_pax_restricted(archive_ptr) != ARCHIVE_OK) {
      throw std::runtime_error("Failed to set archive format: " +
                               std::string(archive_error_string(archive_ptr)));
    }

    if (archive_write_open_filename(archive_ptr, archive_file.c_str()) !=
        ARCHIVE_OK) {
      throw std::runtime_error("Failed to open output archive: " +
                               std::string(archive_error_string(archive_ptr)));
    }

    FullBackup::AddDirectoryToArchive(archive_ptr, fs::path(source_path),
                          fs::path(source_path));

    archive_write_close(archive_ptr);
    archive_write_free(archive_ptr);
    // Generate metadata
    utils::MetadataLogger::GenerateMetadata(source_path, metadata_file);

    // Log backup archive name
    utils::BackupLogger::AppendToBackupLog(log_file, archive_file);

    std::cout << "Full backup completed: " << archive_file << "\n";
    std::cout << "Metadata written to: " << metadata_file << "\n";
    std::cout << "Log updated: " << log_file << "\n";

  } catch (const std::exception& ex) {
    std::cerr << "Backup failed: " << ex.what() << std::endl;
  }
}

}  // namespace backup