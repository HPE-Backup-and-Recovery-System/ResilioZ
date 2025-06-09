#pragma once

#include "chunker.hpp"
#include "backup.hpp"
#include "progress.hpp"
#include <filesystem>
#include <string>
#include <vector>
#include <unordered_map>
#include <optional>
#include <map>
#include <chrono>
#include <nlohmann/json.hpp>



class Restore {
public:
    Restore(const std::filesystem::path& input_path,
            const std::filesystem::path& output_path,
            const std::string& backup_name);
    
    // Restore a single file
    void restore_file(const std::string& filename);
    
    // Restore all files from backup
    void restore_all();
    
    // List available backups
    std::vector<std::string> list_backups();
    
    // Compare two backups
    void compare_backups(const std::string& backup1, const std::string& backup2);

    void restore(const std::filesystem::path& backup_path,
                const std::filesystem::path& restore_path,
                const std::string& backup_id);

private:
    // Load metadata from backup
    void load_metadata();
    
    // Load a chunk from disk
    Chunk load_chunk(const std::string& hash);
    
    // Decompress a chunk using zstd
    Chunk decompress_chunk(const Chunk& compressed_chunk);
    
    // Helper methods for file restore
    std::optional<std::pair<std::string, FileMetadata>> find_file_metadata(const std::string& filename);
    std::filesystem::path prepare_output_path(const std::string& filename, const std::string& original_path);
    Chunk get_next_chunk(const FileMetadata& file_metadata, ProgressBar& progress);
    
    std::filesystem::path input_path_;
    std::filesystem::path output_path_;
    std::string backup_name_;
    BackupMetadata metadata_;
    Chunker chunker_;
};

