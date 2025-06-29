#ifndef RESTORE_HPP_
#define RESTORE_HPP_

#include <chrono>
#include <filesystem>
#include <map>
#include <nlohmann/json.hpp>
#include <optional>
#include <string>
#include <unordered_map>
#include <vector>

#include "backup.hpp"
#include "chunker.hpp"
#include "progress.hpp"

namespace fs = std::filesystem;

class Restore {
 public:
  Restore(Repository* repo);
  ~Restore();
  // Restore a single file
  void RestoreFile(const fs::path & file_path,const fs::path output_path_, const std::string backup_name_);

  // Restore all files from backup
  void RestoreAll(const fs::path output_path_, const std::string backup_name_);

  // List available backups
  std::vector<std::string> ListBackups();

  // Compare two backups
  void CompareBackups(const std::string& backup1, const std::string& backup2);

  
  protected:
  // Load metadata from backup
  void LoadMetadata(const std::string backup_name_);
  bool CheckFileIntegrity(const fs::path& file_path, const std::string& expected_checksum);
  std::pair<std::string,int> ReportResults();
  Repository* repo_;
  std::optional<BackupMetadata*> metadata_ = nullptr;
  fs::path temp_dir_;
  std::vector<std::string> integrity_failures_; // Track files that failed integrity check
  std::vector<std::string> failed_files_; // Track files that failed to restore
  std::vector<std::string> successful_files_; // Track files that succeeded
  

 private:

  // Load a chunk from disk
  Chunk LoadChunk(const std::string& hash);

  // Decompress a chunk using zstd
  Chunk DecompressChunk(const Chunk& compressed_chunk);

  // Helper methods for file restore
  std::optional<std::pair<std::string, FileMetadata>> FindFileMetadata(
      const std::string& filename);
  fs::path PrepareOutputPath(const std::string& filename,
                             const fs::path & original_path,
                             const fs::path output_path_);
  Chunk GetNextChunk(const FileMetadata& file_metadata, ProgressBar& progress);

  Chunker chunker_;

  void SetFilePermissions(const fs::path& file_path, const std::string& permissions);
  std::string CalculateFileSHA256(const fs::path& file_path);
  
  // Chunk tracking for GetNextChunk
  size_t current_chunk_ = 0;
  size_t processed_bytes_ = 0;
  std::string current_file_hash_; // Track which file we're processing
};

#endif  // RESTORE_HPP_
