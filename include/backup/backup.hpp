#pragma once
#include "chunker.hpp"
#include <filesystem>
#include <string>
#include <vector>
#include <map>
#include <chrono>
#include <nlohmann/json.hpp>
#include "progress.hpp"



enum class BackupType {
    FULL,
    INCREMENTAL,
    DIFFERENTIAL
};

struct FileMetadata {
    std::string original_filename;
    std::vector<std::string> chunk_hashes;
    uint64_t total_size;
    std::filesystem::file_time_type mtime;
};

struct BackupMetadata {
    BackupType type;
    std::chrono::system_clock::time_point timestamp;
    std::string previous_backup;
    std::string remarks;
    std::map<std::string, FileMetadata> files;
};

class Backup {
public:
    Backup(const std::filesystem::path& input_path,
           const std::filesystem::path& output_path,
           BackupType type = BackupType::FULL,
           const std::string& remarks = "",
           size_t average_chunk_size = 8192);

    void backup_directory();

    // Utility functions
    static std::vector<std::string> list_backups(const std::filesystem::path& backup_dir);
    static void print_all_backup_details(const std::filesystem::path& backup_dir);
    static void compare_backups(const std::filesystem::path& backup_dir,
                              const std::string& backup1,
                              const std::string& backup2);

private:
    void backup_file(const std::filesystem::path& file_path);
    bool should_skip_file(const std::filesystem::path& file_path);
    FileMetadata create_file_metadata(const std::filesystem::path& file_path);
    void process_chunk(const Chunk& chunk, FileMetadata& file_metadata, ProgressBar& progress);
    void save_metadata();
    std::string generate_chunk_filename(const std::string& hash);
    Chunk compress_chunk(const Chunk& original_chunk);
    void save_chunk(const Chunk& chunk);
    static BackupMetadata load_previous_metadata(const std::filesystem::path& backup_dir,
                                               const std::string& backup_name);
    static std::string get_latest_backup(const std::filesystem::path& backup_dir);
    static std::string get_latest_full_backup(const std::filesystem::path& backup_dir);
    static bool has_file_changed(const std::filesystem::path& file_path,
                               const FileMetadata& previous_metadata);

    std::filesystem::path input_path_;
    std::filesystem::path output_path_;
    Chunker chunker_;
    BackupType backup_type_;
    BackupMetadata metadata_;
};
