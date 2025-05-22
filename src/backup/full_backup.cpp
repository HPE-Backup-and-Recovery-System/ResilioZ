#include "backup/full_backup.h"

#include <archive.h>
#include <archive_entry.h>
#include <nlohmann/json.hpp>

#include <fstream>
#include <iostream>
#include <filesystem>

namespace fs = std::filesystem;
using json = nlohmann::json;

namespace backup {

namespace {

// Reads source directory from a JSON config file
std::string GetSourcePathFromConfig(const std::string& config_path) {
  std::ifstream file(config_path);
  if (!file.is_open()) {
    throw std::runtime_error("Failed to open config file: " + config_path);
  }

  json config;
  file >> config;
  return config["source_path"];
}

// Recursively writes files and directories to the archive
void AddDirectoryToArchive(struct archive* archive_ptr,
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

}  // namespace

void FullBackup::Execute(const std::string& config_path,
                         const std::string& destination_path) {
  try {
    std::string source_path = GetSourcePathFromConfig(config_path);

    std::string output_file = destination_path + "/full_backup.tar";

    struct archive* archive_ptr = archive_write_new();
    archive_write_set_format_pax_restricted(archive_ptr);  // POSIX
    archive_write_open_filename(archive_ptr, output_file.c_str());

    AddDirectoryToArchive(archive_ptr, fs::path(source_path), fs::path(source_path));

    archive_write_close(archive_ptr);
    archive_write_free(archive_ptr);

    std::cout << "Full backup completed successfully to: " << output_file << std::endl;
  } catch (const std::exception& ex) {
    std::cerr << "Backup failed: " << ex.what() << std::endl;
  }
}

}  // namespace backup
