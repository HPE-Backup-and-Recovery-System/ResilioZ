#include "backup/differential_backup.h"

#include <archive.h>
#include <archive_entry.h>

#include <chrono>
#include <ctime>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <nlohmann/json.hpp>
#include <vector>

#include "utils/backup_logger.h"
#include "utils/metadata_logger.h"
#include "utils/backup_utils.h"

namespace fs = std::filesystem;
using json = nlohmann::json;
using bu = utils::BackupUtils;
namespace backup {



// Load the latest full backup metadata for comparison
json DifferentialBackup::LoadLatestFullMetadata(const std::string& log_file) {
  std::ifstream log_stream(log_file);
  std::string last_line;
  std::string line;

  while (std::getline(log_stream, line)) {
    if (!line.empty()) last_line = line;
  }

  if (last_line.empty()) {
    throw std::runtime_error("No full backup log found.");
  }

  // Construct corresponding metadata file name
  std::string metadata_file = last_line;
  size_t pos = metadata_file.find(".tar.zst");
  if (pos == std::string::npos) {
    throw std::runtime_error("Invalid log entry format.");
  }

  metadata_file.replace(pos, 8, "_metadata.json");

  std::ifstream metadata_stream(metadata_file);
  if (!metadata_stream.is_open()) {
    throw std::runtime_error("Failed to open metadata file: " + metadata_file);
  }

  json metadata;
  metadata_stream >> metadata;
  return metadata;
}

bool DifferentialBackup::IsFileModified(const fs::path& file_path, const fs::path& source_dir,
                    const json& metadata) {
  try {
    auto rel_path = fs::relative(file_path, source_dir).string();

    // Check if metadata has files array
    if (!metadata.contains("files") || !metadata["files"].is_array()) {
      return true;
    }

    // Search for the file in the metadata array
    auto files = metadata["files"];
    auto file_it =
        std::find_if(files.begin(), files.end(), [&rel_path](const json& file) {
          return file.contains("path") && file["path"] == rel_path;
        });

    if (file_it == files.end()) {
      return true;  // File not found in metadata
    }

    // Get current file modification time
    auto ftime = fs::last_write_time(file_path);

    auto sctp =
        std::chrono::time_point_cast<std::chrono::system_clock::duration>(
            ftime - fs::file_time_type::clock::now() +
            std::chrono::system_clock::now());
    std::time_t current_mtime =
        std::chrono::duration_cast<std::chrono::seconds>(
            sctp.time_since_epoch())
            .count();

    // Get stored modification time
    if (!file_it->contains("mtime")) {
      return true;  // No mtime in metadata, consider modified
    }

    std::time_t stored_mtime = (*file_it)["mtime"];

    return current_mtime > stored_mtime;

  } catch (const fs::filesystem_error& e) {
    // If we can't check, assume it's modified
    return true;
  } catch (const std::exception& e) {
    // If we can't check, assume it's modified
    return true;
  }
}
void DifferentialBackup::AddModifiedFilesToArchive(struct archive* archive_ptr,
                               const fs::path& source_dir,
                               const json& previous_metadata) {
  if (!archive_ptr) {
    throw std::invalid_argument("Archive pointer is null");
  }

  try {
    for (const auto& entry : fs::recursive_directory_iterator(source_dir)) {
      if (fs::is_regular_file(entry.path()) &&
          DifferentialBackup::IsFileModified(entry.path(), source_dir, previous_metadata)) {
        fs::path relative_path = fs::relative(entry.path(), source_dir);

        struct archive_entry* archive_entry = archive_entry_new();
        if (!archive_entry) {
          throw std::runtime_error("Failed to create archive entry");
        }

        try {
          archive_entry_set_pathname(archive_entry,
                                     relative_path.string().c_str());
          archive_entry_set_filetype(archive_entry, AE_IFREG);
          archive_entry_set_perm(archive_entry, 0644);
          archive_entry_set_size(archive_entry, fs::file_size(entry.path()));

          // Write header
          int result = archive_write_header(archive_ptr, archive_entry);
          if (result != ARCHIVE_OK) {
            throw std::runtime_error(
                "Failed to write archive header: " +
                std::string(archive_error_string(archive_ptr)));
          }

          // Write file data
          std::ifstream ifs(entry.path(), std::ios::binary);
          if (!ifs) {
            throw std::runtime_error("Cannot open file for reading: " +
                                     entry.path().string());
          }

          std::vector<char> buffer(8192);
          while (ifs.read(buffer.data(), buffer.size()) || ifs.gcount() > 0) {
            if (ifs.gcount() > 0) {
              ssize_t bytes_written =
                  archive_write_data(archive_ptr, buffer.data(), ifs.gcount());
              if (bytes_written < 0) {
                throw std::runtime_error(
                    "Failed to write archive data: " +
                    std::string(archive_error_string(archive_ptr)));
              }
            }
            if (ifs.eof()) break;
            if (ifs.fail() && !ifs.eof()) {
              throw std::runtime_error("Error reading file: " +
                                       entry.path().string());
            }
          }

        } catch (...) {
          archive_entry_free(archive_entry);
          throw;
        }

        archive_entry_free(archive_entry);
      }
    }

  } catch (const fs::filesystem_error& e) {
    throw std::runtime_error("Filesystem error in AddModifiedFilesToArchive: " +
                             std::string(e.what()));
  }
}


void DifferentialBackup::PerformBackup(const std::string& config_path,
                                 const std::string& destination_path) {
  try {
    std::string source_path = bu::GetSourcePathFromConfig(config_path);
    std::string timestamp = bu::GetCurrentTimestamp();
    std::string full_backup_dir = destination_path + "/backups";
    fs::path individual_backup_dir =
        fs::path(full_backup_dir) / ("differential_backup_" + timestamp);
    fs::create_directories(individual_backup_dir);
    std::string archive_file = individual_backup_dir.string() +
                               "/differential_backup_" + timestamp + ".tar.zst";
    std::string metadata_file = individual_backup_dir.string() +
                                "/differential_backup_metadata_" + timestamp +
                                ".json";

    std::string log_file = destination_path + "/backups" + "/full_backups.log";
    json previous_metadata = DifferentialBackup::LoadLatestFullMetadata(log_file);

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

    AddModifiedFilesToArchive(archive_ptr, source_path, previous_metadata);

    archive_write_close(archive_ptr);
    archive_write_free(archive_ptr);

    utils::MetadataLogger::GenerateMetadata(source_path, metadata_file);

    std::cout << "Differential backup completed: " << archive_file << "\n";
    std::cout << "Metadata written to: " << metadata_file << "\n";

  } catch (const std::exception& ex) {
    std::cerr << "Differential backup failed: " << ex.what() << std::endl;
  }
}

}  // namespace backup