#pragma once

#include "file_scanner.h"
#include <string>
#include <vector>
#include <nlohmann/json.hpp>
#include <memory>
#include <chrono>

namespace hpe {
namespace backup {

struct BackupMetadata {
    std::chrono::system_clock::time_point timestamp;
    std::vector<FileMetadata> files;
    std::string metadata_file;
};

class MetadataManager {
public:
    MetadataManager(const std::string& backup_path);
    ~MetadataManager() = default;

    // Save metadata to a timestamped JSON file
    void save_metadata(const std::vector<FileMetadata>& metadata);
    
    // Load the most recent metadata
    std::vector<FileMetadata> load_latest_metadata();
    
    // Load metadata from a specific backup
    std::vector<FileMetadata> load_metadata(const std::string& metadata_file);
    
    // Get list of all backup metadata files
    std::vector<BackupMetadata> get_backup_history();
    
    // Display backup history
    void show_backup_status();
    
    // Get the path to the metadata directory
    std::string get_metadata_path() const;
    
    // Compare current metadata with previous backup metadata
    std::vector<FileMetadata> get_changed_files(
        const std::vector<FileMetadata>& current_metadata,
        const std::vector<FileMetadata>& previous_metadata
    );

private:
    std::string backup_path_;
    std::string metadata_dir_;
    
    // Convert FileMetadata to JSON
    nlohmann::json metadata_to_json(const FileMetadata& metadata);
    
    // Convert JSON to FileMetadata
    FileMetadata json_to_metadata(const nlohmann::json& json);

    // Generate metadata filename with timestamp
    std::string generate_metadata_filename() const;

    // Get timestamp from metadata filename
    std::chrono::system_clock::time_point get_timestamp_from_filename(const std::string& filename) const;

    // Update backup history log
    void update_backup_history(const std::string& metadata_file,
                             const std::vector<FileMetadata>& metadata);
};

} // namespace backup
} // namespace hpe 