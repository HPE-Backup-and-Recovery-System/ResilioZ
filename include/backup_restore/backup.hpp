#ifndef BACKUP_HPP_
#define BACKUP_HPP_

#include <chrono>
#include <filesystem>
#include <map>
#include <nlohmann/json.hpp>
#include <string>
#include <vector>

#include "chunker.hpp"
#include "progress.hpp"

namespace fs = std::filesystem;

enum class BackupType { FULL, INCREMENTAL, DIFFERENTIAL };

struct FileMetadata {
  std::string original_filename;
  std::vector<std::string> chunk_hashes;
  uint64_t total_size;
  fs::file_time_type mtime;
  bool is_symlink = false;
  std::string symlink_target;
};

struct BackupMetadata {
  BackupType type;
  std::chrono::system_clock::time_point timestamp;
  std::string original_path;
  std::string previous_backup;
  std::string remarks;
  std::map<std::string, FileMetadata> files;
};

class Backup {
 public:
  Backup(const fs::path& input_path, const fs::path& output_path,
         BackupType type = BackupType::FULL, const std::string& remarks = "",
         size_t average_chunk_size = 1024 * 1024);

  void BackupDirectory();

  // Utility functions
  static std::vector<std::string> ListBackups(const fs::path& backup_dir);
  static void DisplayAllBackupDetails(const fs::path& backup_dir);
  static void CompareBackups(const fs::path& backup_dir,
                             const std::string& backup1,
                             const std::string& backup2);

 private:
  void BackupFile(const fs::path& file_path);
  bool CheckFileToSkip(const fs::path& file_path);
  FileMetadata CheckFileMetadata(const fs::path& file_path);
  void ProcessChunk(const Chunk& chunk, FileMetadata& file_metadata,
                    ProgressBar& progress);
  void SaveMetadata();
  std::string GenerateChunkFilename(const std::string& hash);
  Chunk CompressChunk(const Chunk& original_chunk);
  void SaveChunk(const Chunk& chunk);
  static BackupMetadata LoadPreviousMetadata(const fs::path& backup_dir,
                                             const std::string& backup_name);
  static std::string GetLatestBackup(const fs::path& backup_dir);
  static std::string GetLatestFullBackup(const fs::path& backup_dir);
  static bool CheckFileForChanges(const fs::path& file_path,
                                  const FileMetadata& previous_metadata);

  fs::path input_path_;
  fs::path output_path_;
  Chunker chunker_;
  BackupType backup_type_;
  BackupMetadata metadata_;
};

#endif  // BACKUP_HPP_
