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
#include <repositories/all.h>

namespace fs = std::filesystem;

enum class BackupType { FULL, INCREMENTAL, DIFFERENTIAL };

struct FileMetadata {
  std::string original_filename;
  std::vector<std::string> chunk_hashes;
  uint64_t total_size;
  fs::file_time_type mtime;
  bool is_symlink = false;
  std::string symlink_target;
  std::string permissions;  // File permissions in octal format (e.g., "0644")
  std::string sha256_checksum;  // SHA256 hash of the entire file
};

struct BackupMetadata {
  BackupType type;
  std::chrono::system_clock::time_point timestamp;
  std::string original_path;
  std::string previous_backup;
  std::string remarks;
  std::map<std::string, FileMetadata> files;
  BackupMetadata(){};
  BackupMetadata(BackupType type_,std::chrono::system_clock::time_point timestamp_,
                std::string previous_backup_,std::map<std::string, FileMetadata> files_):
                type(type_),timestamp(timestamp_),previous_backup(previous_backup_),files(files_) {};
};

struct BackupDetails {
  std::string type;
  std::string timestamp;
  std::string name;
  std::string remarks;
  BackupDetails(const std::string& type, const std::string& timestamp,
                const std::string& name, const std::string& remarks)
      : type(type), timestamp(timestamp), name(name), remarks(remarks) {}
};

class Backup {
 public:
  Backup(Repository* repo, const fs::path& input_path,
         BackupType type = BackupType::FULL, const std::string& remarks = "",
         size_t average_chunk_size = 1024 * 1024);
  ~Backup();
  void BackupDirectory();

  // Utility functions
  std::vector<std::string> ListBackups();
  void DisplayAllBackupDetails();
  void CompareBackups(const std::string& backup1,
                             const std::string& backup2);
  std::vector<BackupDetails> GetAllBackupDetails();

 protected:
  void BackupFile(const fs::path& file_path);
  bool CheckFileToSkip(const fs::path& file_path);
  FileMetadata CheckFileMetadata(const fs::path& file_path);
  void ProcessChunk(const Chunk& chunk, FileMetadata& file_metadata,
                    ProgressBar& progress);
  void SaveMetadata();
  std::string GenerateChunkFilename(const std::string& hash);
  Chunk CompressChunk(const Chunk& original_chunk);
  void SaveChunk(const Chunk& chunk);
  BackupMetadata LoadPreviousMetadata(const std::string& backup_name);
  std::string GetLatestBackup();
  std::string GetLatestFullBackup();
  bool CheckFileForChanges(const fs::path& file_path,
                                  const FileMetadata& previous_metadata);

  fs::path input_path_;
  fs::path temp_dir_;
  Repository* repo_;
  Chunker chunker_;
  BackupType backup_type_;
  BackupMetadata metadata_ ;

 private:
  std::string CalculateFileSHA256(const fs::path& file_path);
  std::string GetFilePermissions(const fs::path& file_path);
};

#endif  // BACKUP_HPP_
