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

 private:
  // Load metadata from backup
  void LoadMetadata(const std::string backup_name_);

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

  Repository* repo_;
  fs::path temp_dir_;
  std::optional<BackupMetadata*> metadata_ = nullptr;
  Chunker chunker_;
};

#endif  // RESTORE_HPP_
