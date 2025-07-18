#include "backup_restore/backup.hpp"

#include <openssl/sha.h>
#include <zstd.h>

#include <algorithm>
#include <filesystem>
#include <fstream>
#include <iomanip>
#include <iostream>
#include <map>
#include <nlohmann/json.hpp>
#include <set>
#include <sstream>

#include "backup_restore/progress.hpp"
#include "utils/error_util.h"
#include "utils/logger.h"
#include "utils/user_io.h"
#include "utils/encryption_util.h"

namespace fs = std::filesystem;

Backup::Backup(Repository* repo, const fs::path& input_path, BackupType type,
               const std::string& remarks, size_t average_chunk_size)
    : input_path_(input_path),
      repo_(repo),
      chunker_(average_chunk_size),
      temp_dir_(fs::temp_directory_path() / ("backup_temp_" + repo->GetName())),
      backup_type_(type) {
  if (!fs::exists(input_path_)) {
    ErrorUtil::ThrowError("Input path does not exist: " + input_path_.string());
  }

  // Create necessary directories

  fs::create_directories(temp_dir_);
  fs::create_directories(temp_dir_ / "backup");
  fs::create_directories(temp_dir_ / "chunks");
  const fs::path prev_meta_path = temp_dir_ / "backup";
  auto fetched_metadata =
      repo_->DownloadDirectory("backup/", prev_meta_path.string());
  if (!fetched_metadata) ErrorUtil::ThrowError("Failed to load metadata");

  // Initialize metadata
  metadata_.type = type;
  metadata_.timestamp = std::chrono::system_clock::now();
  metadata_.remarks = remarks;

  // For incremental/differential backups, load previous metadata
  if (type != BackupType::FULL) {
    std::string previous_backup;
    if (type == BackupType::INCREMENTAL) {
      previous_backup = GetLatestBackup();
    } else {  // DIFFERENTIAL
      previous_backup = GetLatestFullBackup();
    }

    if (previous_backup.empty()) {
      std::string backup_type =
          (type == BackupType::INCREMENTAL) ? "incremental" : "differential";

      ErrorUtil::ThrowError("No suitable previous backup found for " +
                            backup_type +
                            " backup. Please perform a full backup first.");
    }

    metadata_.previous_backup = previous_backup;

    BackupMetadata prev_metadata = LoadPreviousMetadata(previous_backup);
    metadata_.files = prev_metadata.files;
  }
}

Backup::~Backup() {
  if (fs::exists(temp_dir_)) {
    fs::remove_all(temp_dir_);
  }
}

void Backup::BackupFile(const fs::path& file_path) {
  if (CheckFileToSkip(file_path)) {
    return;
  }

  auto file_metadata = CheckFileMetadata(file_path);

  // For symlinks, we don't need to chunk content, just store the metadata
  if (file_metadata.is_symlink) {
    Logger::TerminalLog("Backing up symlink: " + file_path.string() + " -> " +
                        file_metadata.symlink_target);
    metadata_.files[file_path.string()] = file_metadata;
    return;
  }

  ProgressBar progress(file_metadata.total_size, 0,
                       "Backup of " + file_path.string());

  size_t processed_bytes = 0;
  size_t processed_chunks = 0;

  // Use streaming chunking
  chunker_.StreamSplitFile(file_path, [&](const Chunk& chunk) {
    // Compress the chunk before saving
    Chunk compressed_chunk = CompressChunk(chunk);
    file_metadata.chunk_hashes.push_back(compressed_chunk.hash);
    SaveChunk(compressed_chunk);

    // Update progress
    processed_bytes += chunk.size;
    processed_chunks++;
    progress.Update(processed_bytes, processed_chunks);
  });

  progress.Complete();

  metadata_.files[file_path.string()] = file_metadata;
}

bool Backup::CheckFileToSkip(const fs::path& file_path) {
  if (backup_type_ != BackupType::FULL) {
    auto it = metadata_.files.find(file_path.string());
    if (it != metadata_.files.end() &&
        !CheckFileForChanges(file_path, it->second)) {
      Logger::TerminalLog("Skipping unchanged file: " + file_path.string());
      return true;
    }
  }
  return false;
}

FileMetadata Backup::CheckFileMetadata(const fs::path& file_path) {
  FileMetadata metadata;
  metadata.original_filename = file_path.filename().string();

  // Get file permissions
  metadata.permissions = GetFilePermissions(file_path);

  // Check if it's a symlink
  if (fs::is_symlink(file_path)) {
    metadata.is_symlink = true;
    metadata.symlink_target = fs::read_symlink(file_path).string();
    // For symlinks, we don't need to chunk the content, just store the target
    metadata.total_size = 0;
    metadata.mtime = fs::last_write_time(file_path);
    metadata.sha256_checksum = ""; // Symlinks don't have content checksum
  } else {
    metadata.is_symlink = false;
    metadata.total_size = fs::file_size(file_path);
    metadata.mtime = fs::last_write_time(file_path);
    // Calculate SHA256 checksum for regular files
    metadata.sha256_checksum = CalculateFileSHA256(file_path);
  }

  return metadata;
}

void Backup::ProcessChunk(const Chunk& chunk, FileMetadata& file_metadata,
                          ProgressBar& progress) {
  // Compress the chunk before saving
  Chunk compressed_chunk = CompressChunk(chunk);
  file_metadata.chunk_hashes.push_back(compressed_chunk.hash);
  SaveChunk(compressed_chunk);

  // Update progress
  progress.Update(chunk.size, file_metadata.chunk_hashes.size());
}

void Backup::BackupDirectory() {
  size_t changed_files = 0;
  size_t unchanged_files = 0;
  size_t added_files = 0;
  size_t deleted_files = 0;

  // Track files in current backup
  std::set<std::string> current_files;

  // First, check for deleted files

  for (auto it = metadata_.files.begin(); it != metadata_.files.end();) {
    if (!fs::exists(it->first)) {
      deleted_files++;
      it = metadata_.files.erase(it);  // erase returns the next valid iterator
    } else {
      ++it;
    }
  }

  // Then process existing files
  for (const auto& entry : fs::recursive_directory_iterator(input_path_)) {
    // Handle both regular files and symlinks (both file and directory symlinks)
    if (entry.is_regular_file() || fs::is_symlink(entry.path())) {
      std::string file_path = entry.path().string();
      current_files.insert(file_path);

      auto it = metadata_.files.find(file_path);
      if (it == metadata_.files.end()) {
        added_files++;
        BackupFile(entry.path());
      } else if (CheckFileForChanges(entry.path(), it->second)) {
        changed_files++;
        BackupFile(entry.path());
      } else {
        unchanged_files++;
      }
    }
  }

  std::ostringstream summary;
  summary << "Backup Summary:"
          << "\n - Changed files: " << changed_files
          << "\n - Unchanged files: " << unchanged_files
          << "\n - Added files: " << added_files
          << "\n - Deleted files: " << deleted_files << std::endl;
  Logger::TerminalLog(summary.str());

  SaveMetadata();
}

void Backup::SaveMetadata() {
  // Generate backup name from timestamp
  auto time = std::chrono::system_clock::to_time_t(metadata_.timestamp);
  std::stringstream ss;
  ss << std::put_time(std::localtime(&time), "%Y%m%d_%H%M%S");
  std::string backup_name = ss.str();

  // Convert metadata to JSON
  nlohmann::json metadata_json;
  metadata_json["type"] = static_cast<int>(metadata_.type);
  metadata_json["timestamp"] =
      std::chrono::system_clock::to_time_t(metadata_.timestamp);
  metadata_json["previous_backup"] = metadata_.previous_backup;
  metadata_json["remarks"] = metadata_.remarks;

  nlohmann::json files_json;
  for (const auto& [file_path, file_metadata] : metadata_.files) {
    nlohmann::json file_json;
    file_json["original_filename"] = file_metadata.original_filename;
    file_json["chunk_hashes"] = file_metadata.chunk_hashes;
    file_json["total_size"] = file_metadata.total_size;
    file_json["is_symlink"] = file_metadata.is_symlink;
    file_json["permissions"] = file_metadata.permissions;
    file_json["sha256_checksum"] = file_metadata.sha256_checksum;
    if (file_metadata.is_symlink) {
      file_json["symlink_target"] = file_metadata.symlink_target;
    }

    // Convert file times to seconds since epoch
    auto mtime_seconds = std::chrono::duration_cast<std::chrono::seconds>(
                             file_metadata.mtime.time_since_epoch())
                             .count();

    file_json["mtime"] = mtime_seconds;
    files_json[file_path] = file_json;
  }
  metadata_json["files"] = files_json;

  // Encrypt metadata using repository password
  std::string json_string = metadata_json.dump(4);
  std::vector<uint8_t> encrypted_data = EncryptionUtil::EncryptMetadata(json_string, repo_->GetPassword());
  
  // Save metadata
  fs::path local_meta_path = temp_dir_ / "backup" / backup_name;
  std::ofstream metadata_file(local_meta_path, std::ios::binary);
  metadata_file.write(reinterpret_cast<const char*>(encrypted_data.data()), encrypted_data.size());
  metadata_file.close();

  repo_->UploadFile(local_meta_path.string(), "backup/");
}

std::string Backup::GenerateChunkFilename(const std::string& hash) {
  // Use first two hex digits as subdirectory
  std::string subdir = hash.substr(0, 2);
  fs::create_directories(temp_dir_ / "chunks" / subdir);
  return subdir + "/" + hash + ".chunk";
}

Chunk Backup::CompressChunk(const Chunk& original_chunk) {
  // Calculate maximum compressed size
  size_t const max_compressed_size = ZSTD_compressBound(original_chunk.size);

  // Create buffer for compressed data (including size prefix)
  std::vector<uint8_t> compressed_data(sizeof(size_t) + max_compressed_size);

  // Store original size at the beginning
  *reinterpret_cast<size_t*>(compressed_data.data()) = original_chunk.size;

  // Compress the data
  size_t const compressed_bytes = ZSTD_compress(
      compressed_data.data() + sizeof(size_t), max_compressed_size,
      original_chunk.data.data(), original_chunk.size, ZSTD_CLEVEL_DEFAULT);

  if (ZSTD_isError(compressed_bytes)) {
    ErrorUtil::ThrowError("Failed to compress chunk: " +
                          std::string(ZSTD_getErrorName(compressed_bytes)));
  }

  // Resize the vector to actual compressed size + size prefix
  compressed_data.resize(sizeof(size_t) + compressed_bytes);

  // Calculate new hash for compressed chunk
  unsigned char hash[SHA256_DIGEST_LENGTH];
  SHA256_CTX sha256;
  SHA256_Init(&sha256);
  SHA256_Update(&sha256, compressed_data.data(), compressed_data.size());
  SHA256_Final(hash, &sha256);

  // Convert hash to hex string
  std::stringstream ss;
  for (int i = 0; i < SHA256_DIGEST_LENGTH; i++) {
    ss << std::hex << std::setw(2) << std::setfill('0')
       << static_cast<int>(hash[i]);
  }

  Chunk compressed_chunk;
  compressed_chunk.hash = ss.str();
  compressed_chunk.data = std::move(compressed_data);
  compressed_chunk.size = compressed_bytes;  // Store actual compressed size
  return compressed_chunk;
}

void Backup::SaveChunk(const Chunk& chunk) {
  fs::path chunk_path =
      temp_dir_ / "chunks" / GenerateChunkFilename(chunk.hash);

  if (!fs::exists(chunk_path)) {
    std::ofstream chunk_file(chunk_path, std::ios::binary);
    if (!chunk_file) {
      ErrorUtil::ThrowError("Could not create chunk file: " +
                            chunk_path.string());
    }
    chunk_file.write(reinterpret_cast<const char*>(chunk.data.data()),
                     chunk.data.size());
    chunk_file.close();
    const fs::path repo_target = "chunks/" + chunk.hash.substr(0, 2) + "/";
    repo_->UploadFile(chunk_path.string(), repo_target.string());
  }
}

BackupMetadata Backup::LoadPreviousMetadata(const std::string& backup_name) {
  fs::path metadata_path = temp_dir_ / "backup" / backup_name;
  if (!fs::exists(metadata_path)) {
    ErrorUtil::ThrowError("Previous backup metadata not found: " +
                          metadata_path.string());
  }

  std::ifstream metadata_file(metadata_path, std::ios::binary);
  if (!metadata_file) {
    ErrorUtil::ThrowError("Could not open metadata file: " + metadata_path.string());
  }

  // Read encrypted data
  std::vector<uint8_t> encrypted_data(
    (std::istreambuf_iterator<char>(metadata_file)),
    std::istreambuf_iterator<char>()
  );
  metadata_file.close();

  // Decrypt metadata using repository password
  std::string json_string = EncryptionUtil::DecryptMetadata(encrypted_data, repo_->GetPassword());
  if (json_string.empty()) {
    ErrorUtil::ThrowError("Failed to decrypt metadata: " + backup_name);
  }

  nlohmann::json metadata_json = nlohmann::json::parse(json_string);

  BackupMetadata metadata;
  metadata.type = static_cast<BackupType>(metadata_json["type"].get<int>());
  metadata.timestamp = std::chrono::system_clock::from_time_t(
      metadata_json["timestamp"].get<time_t>());
  metadata.previous_backup =
      metadata_json["previous_backup"].get<std::string>();
  metadata.remarks = metadata_json.value("remarks", "");

  for (const auto& [file_path, file_json] : metadata_json["files"].items()) {
    FileMetadata file_metadata;
    file_metadata.original_filename = file_json["original_filename"];
    file_metadata.chunk_hashes =
        file_json["chunk_hashes"].get<std::vector<std::string>>();
    file_metadata.total_size = file_json["total_size"];
    file_metadata.mtime = fs::file_time_type(
        std::chrono::duration_cast<fs::file_time_type::duration>(
            std::chrono::seconds(file_json["mtime"].get<time_t>())));

    // Load symlink information (with backward compatibility)
    file_metadata.is_symlink = file_json.value("is_symlink", false);
    if (file_metadata.is_symlink) {
      file_metadata.symlink_target = file_json["symlink_target"];
    }

    // Load new fields with backward compatibility
    file_metadata.permissions = file_json.value("permissions", "");
    file_metadata.sha256_checksum = file_json.value("sha256_checksum", "");

    metadata.files[file_path] = file_metadata;
  }

  return metadata;
}

std::string Backup::GetLatestBackup() {
  auto backups = ListBackups();
  Logger::TerminalLog("Getting latest backup from " + temp_dir_.string());
  return backups.empty() ? "" : backups[0];
}

std::string Backup::GetLatestFullBackup() {
  auto backups = ListBackups();
  for (auto it = backups.begin(); it != backups.end(); ++it) {
    fs::path metadata_path = temp_dir_ / "backup" / *it;
    std::ifstream metadata_file(metadata_path, std::ios::binary);
    if (!metadata_file) {
      continue;
    }

    // Read encrypted data
    std::vector<uint8_t> encrypted_data(
      (std::istreambuf_iterator<char>(metadata_file)),
      std::istreambuf_iterator<char>()
    );
    metadata_file.close();

    // Decrypt metadata using repository password
    std::string json_string = EncryptionUtil::DecryptMetadata(encrypted_data, repo_->GetPassword());
    if (json_string.empty()) {
      continue; // Skip corrupted metadata
    }

    try {
      nlohmann::json metadata_json = nlohmann::json::parse(json_string);
      if (static_cast<BackupType>(metadata_json["type"].get<int>()) == BackupType::FULL) {
        return *it;
      }
    } catch (...) {
      continue; // Skip invalid JSON
    }
  }
  return "";
}

std::vector<std::string> Backup::ListBackups() {
  std::vector<std::string> backups;
  for (const auto& entry : fs::directory_iterator(temp_dir_ / "backup")) {
    if (entry.is_regular_file()) {
      backups.push_back(entry.path().filename().string());
    }
  }

  // Return backups in descending order
  std::sort(backups.rbegin(), backups.rend());

  return backups;
}

std::vector<BackupDetails> Backup::GetAllBackupDetails() {
  std::vector<BackupDetails> backupDetails;
  const auto backups = ListBackups();
  for (const auto& backup : backups) {
    fs::path metadata_path = temp_dir_ / "backup" / backup;
    std::ifstream metadata_file(metadata_path, std::ios::binary);
    if (!metadata_file) {
      continue;
    }

    // Read encrypted data
    std::vector<uint8_t> encrypted_data(
      (std::istreambuf_iterator<char>(metadata_file)),
      std::istreambuf_iterator<char>()
    );
    metadata_file.close();

    // Decrypt metadata using repository password
    std::string json_string = EncryptionUtil::DecryptMetadata(encrypted_data, repo_->GetPassword());
    if (json_string.empty()) {
      continue; // Skip corrupted metadata
    }

    try {
      nlohmann::json metadata_json = nlohmann::json::parse(json_string);

      auto type = static_cast<BackupType>(metadata_json["type"].get<int>());
      auto timestamp = std::chrono::system_clock::from_time_t(
          metadata_json["timestamp"].get<time_t>());
      auto time = std::chrono::system_clock::to_time_t(timestamp);
      std::stringstream timestamp_str;
      timestamp_str << std::put_time(std::localtime(&time), "%Y-%m-%d %H:%M:%S");

      std::string type_str;
      switch (type) {
        case BackupType::FULL:
          type_str = "FULL";
          break;
        case BackupType::INCREMENTAL:
          type_str = "INCREMENTAL";
          break;
        case BackupType::DIFFERENTIAL:
          type_str = "DIFFERENTIAL";
          break;
      }

      std::string remarks = metadata_json.value("remarks", "");
      BackupDetails details(type_str, timestamp_str.str(), backup, remarks);
      backupDetails.push_back(details);
    } catch (...) {
      continue; // Skip invalid JSON
    }
  }
  return backupDetails;
}

void Backup::DisplayAllBackupDetails() {
  const auto backupDetails = GetAllBackupDetails();

  if (backupDetails.empty()) {
    UserIO::DisplayMinTitle("No backups found");
    return;
  }

  UserIO::DisplayMinTitle("Backup List");
  std::cout << std::setw(20) << "Name" << " | " << std::setw(10) << "Type"
            << " | " << std::setw(20) << "Time" << " | " << std::setw(30)
            << "Remarks" << " | \n";
  std::cout << std::string(90, '-') << std::endl;

  for (const auto& backup : backupDetails) {
    std::string remarks = backup.remarks;
    if (remarks.length() > 27) {
      remarks = remarks.substr(0, 24) + "...";
    }

    std::cout << std::setw(20) << backup.name << " | " << std::setw(10)
              << backup.type << " | " << std::setw(20) << backup.timestamp
              << " | " << std::setw(30) << remarks << " | \n";
  }
  std::cout << std::string(90, '-') << "\n\n";
}

void Backup::CompareBackups(const std::string& backup1,
                            const std::string& backup2) {
  BackupMetadata metadata1 = LoadPreviousMetadata(backup1);
  BackupMetadata metadata2 = LoadPreviousMetadata(backup2);

  size_t changed_files = 0;
  size_t unchanged_files = 0;
  size_t added_files = 0;
  size_t deleted_files = 0;

  // Compare files
  for (const auto& [file_path, file_metadata2] : metadata2.files) {
    auto it = metadata1.files.find(file_path);
    if (it == metadata1.files.end()) {
      added_files++;
    } else if (file_metadata2.total_size != it->second.total_size ||
               file_metadata2.mtime != it->second.mtime) {
      changed_files++;
    } else {
      unchanged_files++;
    }
  }

  // Count deleted files
  for (const auto& [file_path, _] : metadata1.files) {
    if (metadata2.files.find(file_path) == metadata2.files.end()) {
      deleted_files++;
    }
  }

  std::ostringstream comparison;
  comparison << "Backup Comparison (" << backup1 << " vs " << backup2 << "):"
             << "\n - Changed files: " << changed_files
             << "\n - Unchanged files: " << unchanged_files
             << "\n - Added files: " << added_files
             << "\n - Deleted files: " << deleted_files << std::endl;
  Logger::TerminalLog(comparison.str());
}

bool Backup::CheckFileForChanges(const fs::path& file_path,
                                 const FileMetadata& previous_metadata) {
  // Check if it's a symlink
  if (fs::is_symlink(file_path)) {
    std::string current_target = fs::read_symlink(file_path).string();
    auto mtime = fs::last_write_time(file_path);
    std::string current_permissions = GetFilePermissions(file_path);

    // Convert both times to seconds for comparison
    auto current_mtime_seconds =
        std::chrono::duration_cast<std::chrono::seconds>(
            mtime.time_since_epoch())
            .count();
    auto previous_mtime_seconds =
        std::chrono::duration_cast<std::chrono::seconds>(
            previous_metadata.mtime.time_since_epoch())
            .count();

    // For symlinks, check if target or modification time changed
    return current_target != previous_metadata.symlink_target ||
           current_mtime_seconds != previous_mtime_seconds;
  }

  // Regular file check
  // auto file_status = fs::status(file_path);
  auto mtime = fs::last_write_time(file_path);
  auto size = fs::file_size(file_path);

  // Convert both times to seconds for comparison
  auto current_mtime_seconds =
      std::chrono::duration_cast<std::chrono::seconds>(mtime.time_since_epoch())
          .count();
  auto previous_mtime_seconds =
      std::chrono::duration_cast<std::chrono::seconds>(
          previous_metadata.mtime.time_since_epoch())
          .count();

  return size != previous_metadata.total_size ||
         current_mtime_seconds != previous_mtime_seconds;
}

std::string Backup::CalculateFileSHA256(const fs::path& file_path) {
  std::ifstream file(file_path, std::ios::binary);
  if (!file) {
    ErrorUtil::ThrowError("Could not open file for SHA256 calculation: " + file_path.string());
  }

  SHA256_CTX sha256;
  SHA256_Init(&sha256);

  char buffer[4096];
  while (file.read(buffer, sizeof(buffer))) {
    SHA256_Update(&sha256, buffer, file.gcount());
  }
  SHA256_Update(&sha256, buffer, file.gcount()); // Read remaining bytes

  unsigned char hash[SHA256_DIGEST_LENGTH];
  SHA256_Final(hash, &sha256);

  // Convert hash to hex string
  std::stringstream ss;
  for (int i = 0; i < SHA256_DIGEST_LENGTH; i++) {
    ss << std::hex << std::setw(2) << std::setfill('0') << static_cast<int>(hash[i]);
  }

  return ss.str();
}

std::string Backup::GetFilePermissions(const fs::path& file_path) {
  std::error_code ec;
  auto perms = fs::status(file_path, ec).permissions();
  
  if (ec) {
    ErrorUtil::ThrowError("Could not get file permissions: " + file_path.string());
  }

  // Convert permissions to octal format
  std::string perms_str;
  perms_str += (perms & fs::perms::owner_read) != fs::perms::none ? "r" : "-";
  perms_str += (perms & fs::perms::owner_write) != fs::perms::none ? "w" : "-";
  perms_str += (perms & fs::perms::owner_exec) != fs::perms::none ? "x" : "-";
  perms_str += (perms & fs::perms::group_read) != fs::perms::none ? "r" : "-";
  perms_str += (perms & fs::perms::group_write) != fs::perms::none ? "w" : "-";
  perms_str += (perms & fs::perms::group_exec) != fs::perms::none ? "x" : "-";
  perms_str += (perms & fs::perms::others_read) != fs::perms::none ? "r" : "-";
  perms_str += (perms & fs::perms::others_write) != fs::perms::none ? "w" : "-";
  perms_str += (perms & fs::perms::others_exec) != fs::perms::none ? "x" : "-";

  // Convert to octal
  int octal = 0;
  if (perms_str[0] == 'r') octal |= 0400;
  if (perms_str[1] == 'w') octal |= 0200;
  if (perms_str[2] == 'x') octal |= 0100;
  if (perms_str[3] == 'r') octal |= 0040;
  if (perms_str[4] == 'w') octal |= 0020;
  if (perms_str[5] == 'x') octal |= 0010;
  if (perms_str[6] == 'r') octal |= 0004;
  if (perms_str[7] == 'w') octal |= 0002;
  if (perms_str[8] == 'x') octal |= 0001;

  std::stringstream ss;
  ss << std::oct << std::setw(4) << std::setfill('0') << octal;
  return ss.str();
}
