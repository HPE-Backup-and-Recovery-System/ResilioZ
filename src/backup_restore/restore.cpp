#include "backup_restore/restore.hpp"

#include <openssl/sha.h>
#include <zstd.h>

#include <filesystem>
#include <fstream>
#include <iomanip>
#include <iostream>
#include <nlohmann/json.hpp>
#include <sstream>

#include "backup_restore/chunker.hpp"
#include "backup_restore/progress.hpp"
#include "utils/utils.h"

namespace fs = std::filesystem;

Restore::Restore(Repository* repo)
    : repo_(repo) {

  // Create a temporary working directory
  temp_dir_ = fs::temp_directory_path() / ("restore_temp_" + repo->GetName());
    
  // Create necessary directories
  fs::create_directories(temp_dir_);
  fs::create_directories(temp_dir_ / "backup");
  fs::create_directories(temp_dir_ / "chunks");
  const fs::path prev_meta_path = temp_dir_ / "backup";
  auto fetched_metadata = repo_->DownloadDirectory("backup/", prev_meta_path.string());
  if(!fetched_metadata) ErrorUtil::ThrowError("Failed to load metadata");
  
}

Restore::~Restore() {
  if (fs::exists(temp_dir_)) {
    fs::remove_all(temp_dir_);
  }
}

void Restore::LoadMetadata(const std::string backup_name_) {
  try{
    fs::path metadata_path = temp_dir_ / "backup" / backup_name_;

    if (!fs::exists(metadata_path)) {
      ErrorUtil::ThrowError("Backup metadata not found: " +
                            metadata_path.string());
    }

    std::ifstream metadata_file(metadata_path);
    nlohmann::json metadata_json;
    metadata_file >> metadata_json;

    
    BackupType type = static_cast<BackupType>(metadata_json["type"].get<int>());
    std::chrono::system_clock::time_point timestamp = std::chrono::system_clock::from_time_t(
        metadata_json["timestamp"].get<time_t>());
    std::string previous_backup =
        metadata_json["previous_backup"].get<std::string>();
    std::map<std::string, FileMetadata> files;
    for (const auto& [file_path, file_json] : metadata_json["files"].items()) {
      FileMetadata file_metadata;
      file_metadata.original_filename = file_json["original_filename"];
      file_metadata.chunk_hashes =
          file_json["chunk_hashes"].get<std::vector<std::string>>();
      file_metadata.total_size = file_json["total_size"].get<uint64_t>();
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
      
    files[file_path] = file_metadata;
    }
    metadata_ = new BackupMetadata(type,timestamp,previous_backup,files);
  }
  catch (const std::exception& e) {
    ErrorUtil::ThrowError("Failed to load metadata: " + std::string(e.what()));
    throw;
  }
}

void Restore::RestoreFile(const std::filesystem::path & file_path, const fs::path output_path_, const std::string backup_name_) {
  try {
      // Get file metadata
    LoadMetadata(backup_name_);
    auto file_metadata = FindFileMetadata(file_path.string());
    if (!file_metadata) {
      ErrorUtil::ThrowError("File not found in backup: " + file_path.string());
    }

    std::string filename = file_metadata->second.original_filename;
    fs::path output_file = PrepareOutputPath(filename, file_path, output_path_);

    // Handle symlinks
    if (file_metadata->second.is_symlink) {
      Logger::TerminalLog("Restoring symlink: " + output_file.string() + " -> " + file_metadata->second.symlink_target);
      
      // Create the symlink
      fs::create_symlink(file_metadata->second.symlink_target, output_file);
      
      // Restore file metadata (timestamps)
      fs::last_write_time(output_file, file_metadata->second.mtime);
      
      // Restore file permissions if available
      if (!file_metadata->second.permissions.empty()) {
        SetFilePermissions(output_file, file_metadata->second.permissions);
      }
      
      // For symlinks, we don't need to check integrity since they don't have content checksums
      successful_files_.push_back(file_path.string());
      return;
    }

    ProgressBar progress(file_metadata->second.total_size,
                        file_metadata->second.chunk_hashes.size(),
                        "restore of " + filename);

    // Use streaming chunk combining
    try{
      chunker_.StreamCombineChunks(
        [&]() -> Chunk { 
          try {
            return GetNextChunk(file_metadata->second, progress);
          } 
          catch(const std::exception& e) {
            ErrorUtil::ThrowError("Failed to get next chunk: " + std::string(e.what()));
            throw;
          }
         },
        output_file, file_metadata->second.total_size);

      progress.Complete();
    }
    catch (const std::exception& e) {
      ErrorUtil::ThrowError("Failed to combine chunks for file: " + file_path.string() + " - " + e.what());
      throw;
    }


    // Restore file metadata
    fs::last_write_time(output_file, file_metadata->second.mtime);
    
    // Restore file permissions if available
    if (!file_metadata->second.permissions.empty()) {
      SetFilePermissions(output_file, file_metadata->second.permissions);
    }
    
    // Check file integrity
    if (!CheckFileIntegrity(output_file, file_metadata->second.sha256_checksum)) {
      Logger::Log("File integrity check failed for " + output_file.string(),LogLevel::WARNING);
      integrity_failures_.push_back(output_file.string());
      return;
    }
    successful_files_.push_back(output_file.string());
  }
  catch(const std::exception& e) {
    ErrorUtil::ThrowError("Failed to restore file: " + file_path.string() + " - " + e.what());
    throw;
  }

}

std::optional<std::pair<std::string, FileMetadata>> Restore::FindFileMetadata(
    const std::string& file_path) {
    auto it = std::find_if((*metadata_)->files.begin(), (*metadata_)->files.end(),
                         [&file_path](const auto& pair) {
                           return pair.first == file_path;
                         });

  if (it == (*metadata_)->files.end()) {
    return std::nullopt;
  }

  return std::make_pair(it->first, it->second);
}

fs::path Restore::PrepareOutputPath(const std::string& filename,
                                    const fs::path& original_path,
                                    const fs::path output_path_) {

  // Get the parent path from the original file path
  fs::path parent_path = fs::path(original_path).parent_path();

  // Create the parent directories inside the output path
  fs::path output_parent = output_path_;

  // Modify parent path to remove beginning /
  std::string path = parent_path.string();
  output_parent.append(path.substr(1, path.size() - 1));

  // Create parent path in output path
  fs::create_directories(output_parent);

  // Create the final output path using the original filename
  return output_parent / filename;
}

Chunk Restore::GetNextChunk(const FileMetadata& file_metadata,
                            ProgressBar& progress) {
  try{
      // Reset state if we're processing a different file
      std::string new_file_hash = file_metadata.chunk_hashes.empty() ? "" : file_metadata.chunk_hashes[0];
      if (current_file_hash_ != new_file_hash) {
        current_chunk_ = 0;
        processed_bytes_ = 0;
        current_file_hash_ = new_file_hash;
      }

      if (current_chunk_ >= file_metadata.chunk_hashes.size()) {
        current_chunk_ = 0;
        processed_bytes_ = 0;
        current_file_hash_ = "";
        return Chunk{};  // Return empty chunk to signal end
      }

      // Load and decompress the next chunk
      Chunk compressed_chunk = LoadChunk(file_metadata.chunk_hashes[current_chunk_]);
      Chunk decompressed_chunk = DecompressChunk(compressed_chunk);
 
      // Update progress
      processed_bytes_ += decompressed_chunk.size;
      current_chunk_++;
      progress.Update(processed_bytes_, current_chunk_);

      return decompressed_chunk;
  }
  catch (const std::exception& e) {
    ErrorUtil::ThrowError("Failed to get next chunk: " + std::string(e.what()));
    throw;
  }

}

void Restore::RestoreAll(const fs::path output_path_,const std::string backup_name_) {
  try {
      // Ensure output path exists
      if(!fs::exists(output_path_)) fs::create_directories(output_path_);
      
      // Clear previous integrity failures
      integrity_failures_.clear();
      failed_files_.clear();
      successful_files_.clear();
      
      // Reset chunk tracking state
      current_chunk_ = 0;
      processed_bytes_ = 0;
      current_file_hash_ = "";
      
      LoadMetadata(backup_name_);
      
      for (const auto& [file_path, metadata] : (*metadata_)->files) {
        try {
          RestoreFile(file_path, output_path_, backup_name_);
        } catch (const std::exception& e) {
          Logger::Log("Failed to restore file: " + file_path + " - " + e.what(), LogLevel::ERROR);
          failed_files_.push_back(file_path);
        }
      }
      // Report any integrity failures
      auto result = ReportResults();
      if(result.second == 2){
          Logger::Log(result.first, LogLevel::ERROR);
      }
      else if (result.second==1) {
          Logger::Log(result.first, LogLevel::WARNING);
      }
      else Logger::Log(result.first, LogLevel::INFO);

  }
  catch(const std::exception& e) {
    ErrorUtil::ThrowError("Failed to restore all files: " + std::string(e.what()));
  }
}

Chunk Restore::LoadChunk(const std::string& hash) {
  try {
    // Use first two hex digits as subdirectory
    std::string subdir = hash.substr(0, 2);
    fs::path chunk_path = temp_dir_/ "chunks" / subdir / (hash + ".chunk");
    if(!fs::exists(chunk_path.parent_path())) fs::create_directories(chunk_path.parent_path());
    if(!fs::exists(chunk_path)) repo_->DownloadFile("chunks/" + subdir + "/" + hash + ".chunk", chunk_path.string());

    std::ifstream chunk_file(chunk_path, std::ios::binary);
    if (!chunk_file) {
      ErrorUtil::ThrowError("Could not open chunk file: " + chunk_path.string());
    }

    Chunk chunk;
    chunk.hash = hash;
    chunk.data =
        std::vector<uint8_t>((std::istreambuf_iterator<char>(chunk_file)),
                            std::istreambuf_iterator<char>());
    chunk.size = chunk.data.size();

    return chunk;
  }
  catch (const std::exception& e) {
    ErrorUtil::ThrowError("Failed to load chunk: " + hash + " - " + e.what());
    throw;
  }

}

Chunk Restore::DecompressChunk(const Chunk& compressed_chunk) {
  try{
      // Get decompressed size from the first 8 bytes
      size_t decompressed_size =
          *reinterpret_cast<const size_t*>(compressed_chunk.data.data());

      // Create output buffer for decompressed data
      std::vector<uint8_t> decompressed_data(decompressed_size);

      // Decompress the data
      size_t const decompressed_bytes =
          ZSTD_decompress(decompressed_data.data(), decompressed_size,
                          compressed_chunk.data.data() + sizeof(size_t),
                          compressed_chunk.data.size() - sizeof(size_t));

      if (ZSTD_isError(decompressed_bytes)) {
        ErrorUtil::ThrowError("Failed to decompress chunk: " +
                              std::string(ZSTD_getErrorName(decompressed_bytes)));
      }

      Chunk decompressed_chunk;
      decompressed_chunk.hash = compressed_chunk.hash;
      decompressed_chunk.data = std::move(decompressed_data);
      decompressed_chunk.size = decompressed_bytes;

      return decompressed_chunk;
  }
  catch (const std::exception& e) {
    ErrorUtil::ThrowError("Failed to decompress chunk: " + compressed_chunk.hash + " - " + e.what());
    throw;
  }

}

std::vector<std::string> Restore::ListBackups() {
  try{
      std::vector<std::string> backups;
      for (const auto& entry : fs::directory_iterator(temp_dir_ / "backup")) {
        if (entry.is_regular_file()) {
          backups.push_back(entry.path().filename().string());
        }
      }
      std::sort(backups.begin(), backups.end());
  return backups;
  } catch (const std::exception& e) {
    ErrorUtil::ThrowError("Failed to list backups: " + std::string(e.what()));
  }

}

void Restore::CompareBackups(const std::string& backup1,
                             const std::string& backup2) {
  // Load both backup metadata
  fs::path metadata_path1 = temp_dir_ / "backup" / backup1;
  fs::path metadata_path2 = temp_dir_ / "backup" / backup2;

  if (!fs::exists(metadata_path1) || !fs::exists(metadata_path2)) {
    ErrorUtil::ThrowError("One or both backup metadata files not found");
  }

  std::ifstream metadata_file1(metadata_path1);
  std::ifstream metadata_file2(metadata_path2);
  nlohmann::json metadata_json1, metadata_json2;
  metadata_file1 >> metadata_json1;
  metadata_file2 >> metadata_json2;

  BackupMetadata metadata1, metadata2;
  metadata1.type = static_cast<BackupType>(metadata_json1["type"].get<int>());
  metadata2.type = static_cast<BackupType>(metadata_json2["type"].get<int>());

  size_t changed_files = 0;
  size_t unchanged_files = 0;
  size_t added_files = 0;
  size_t deleted_files = 0;

  // Compare files
  for (const auto& [file_path, file_metadata2] :
       metadata_json2["files"].items()) {
    auto it = metadata_json1["files"].find(file_path);
    if (it == metadata_json1["files"].end()) {
      added_files++;
    } else if (file_metadata2["total_size"] != it.value()["total_size"] ||
               file_metadata2["mtime"] != it.value()["mtime"]) {
      changed_files++;
    } else {
      unchanged_files++;
    }
  }

  // Count deleted files
  for (const auto& [file_path, _] : metadata_json1["files"].items()) {
    if (metadata_json2["files"].find(file_path) ==
        metadata_json2["files"].end()) {
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

void Restore::SetFilePermissions(const fs::path& file_path, const std::string& permissions) {
  if (permissions.empty()) {
    return; // Skip if no permissions stored
  }

  try {
    // Convert octal string to integer
    int perms = std::stoi(permissions, nullptr, 8);
    
    // Convert to filesystem permissions
    fs::perms fs_perms = fs::perms::none;
    if (perms & 0400) fs_perms |= fs::perms::owner_read;
    if (perms & 0200) fs_perms |= fs::perms::owner_write;
    if (perms & 0100) fs_perms |= fs::perms::owner_exec;
    if (perms & 0040) fs_perms |= fs::perms::group_read;
    if (perms & 0020) fs_perms |= fs::perms::group_write;
    if (perms & 0010) fs_perms |= fs::perms::group_exec;
    if (perms & 0004) fs_perms |= fs::perms::others_read;
    if (perms & 0002) fs_perms |= fs::perms::others_write;
    if (perms & 0001) fs_perms |= fs::perms::others_exec;
    
    fs::permissions(file_path, fs_perms);
  } catch (const std::exception& e) {
    Logger::Log("Could not set file permissions for " + file_path.string() + ": " + e.what(),LogLevel::WARNING);
  }
}

std::string Restore::CalculateFileSHA256(const fs::path& file_path) {
  try {
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
  catch(const std::exception& e) {
    ErrorUtil::ThrowError("Failed to calculate SHA256 for file: " + file_path.string() + " - " + e.what());
  }

}

bool Restore::CheckFileIntegrity(const fs::path& file_path, const std::string& expected_checksum) {
  try{
    if (expected_checksum.empty()) {
      return true; // Skip check if no checksum stored (e.g., symlinks)
    }
    std::string actual_checksum = CalculateFileSHA256(file_path);
    return actual_checksum == expected_checksum;
  }
  catch (const std::exception& e) {
    ErrorUtil::ThrowError("Failed to check file integrity for " + file_path.string() + " - " + e.what());
    throw;
  }
}

std::pair<std::string,int> Restore::ReportResults() {
  int status = 0; // 0 = OK, 1 = Integrity failures, 2 = Failed files


  int total_files = (*metadata_)->files.size();
  int processed_files = successful_files_.size() + failed_files_.size() + integrity_failures_.size();
  int restored_files = successful_files_.size() + integrity_failures_.size();
  int failed_files_count = failed_files_.size();
  int integrity_failures_count = integrity_failures_.size();
  
  std::ostringstream out;
  out << "Restore Summary:\n"
      << " - Total files: " << total_files << "\n"
      << " - Processed files: " << processed_files << "\n"
      << " - Restored files: " << restored_files << "\n"
      << " - Failed integrity checks: " << integrity_failures_count <<"\n"
      << " - Failed to restore: " << failed_files_count << "\n";
  
  if (failed_files_count  > 0)  {
    status = 2;
    out << " - Status: Restore incomplete\n";
  }
  else if (integrity_failures_count > 0) {
    status = 1;
    out << " - Status: Restore completed with integrity failures\n";
  }
  else out << " - Status: Restore OK\n";

  if (failed_files_count > 0) {
      out << " - Missing files \n";
      for (std::string file_name_ : failed_files_) {
        out << " --  " << file_name_ << " \n";
      }
  }

  if(integrity_failures_count > 0) {
    out << " - Integrity failures \n";
    for (std::string file_name_ : integrity_failures_) {
      out << " --  " << file_name_ << " \n";
    }
  }

  return {out.str(), status};

}
