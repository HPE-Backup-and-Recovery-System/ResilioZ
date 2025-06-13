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
  Restore(const fs::path& input_path, const fs::path& output_path,
          const std::string& backup_name);

  // Static method to load Original Path without creating a Restore object
  static std::string LoadOriginalPath(const fs::path& input_path, 
                                         const std::string& backup_name);

  // Restore a single file
  void RestoreFile(const std::string& filename);

  // Restore all files from backup
  void RestoreAll();

  // List available backups
  std::vector<std::string> ListBackups();

  // Compare two backups
  void CompareBackups(const std::string& backup1, const std::string& backup2);

  void RestoreBackup(const fs::path& backup_path, const fs::path& restore_path,
               const std::string& backup_id);

  // Get backup metadata
  const BackupMetadata& GetMetadata() const { return metadata_; }

 private:
  // Load metadata from backup
  void LoadMetadata();

  // Load a chunk from disk
  Chunk LoadChunk(const std::string& hash);

  // Decompress a chunk using zstd
  Chunk DecompressChunk(const Chunk& compressed_chunk);

  // Helper methods for file restore
  std::optional<std::pair<std::string, FileMetadata>> FindFileMetadata(
      const std::string& filename);
  fs::path PrepareOutputPath(const std::string& filename,
                             const std::string& original_path);
  Chunk GetNextChunk(const FileMetadata& file_metadata, ProgressBar& progress);

  fs::path input_path_;
  fs::path output_path_;
  std::string backup_name_;
  BackupMetadata metadata_;
  Chunker chunker_;
};

#endif  // RESTORE_HPP_
