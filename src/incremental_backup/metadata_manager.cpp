#include "metadata_manager.h"
#include <fstream>
#include <stdexcept>
#include <algorithm>
#include <filesystem>
#include <sstream>
#include <iomanip>
#include <iostream>

namespace fs = std::filesystem;

namespace hpe {
namespace backup {

MetadataManager::MetadataManager(const std::string& backup_path)
    : backup_path_(backup_path),
      metadata_dir_(backup_path + "/file_metadata") {
    
    // Create backup and metadata directories if they don't exist
    fs::create_directories(backup_path_);
    fs::create_directories(metadata_dir_);
}

void MetadataManager::save_metadata(const std::vector<FileMetadata>& metadata) {
    // Generate filename with current timestamp
    std::string filename = generate_metadata_filename();
    std::string filepath = metadata_dir_ + "/" + filename;

    nlohmann::json json_array = nlohmann::json::array();
    
    // Sort metadata by path for consistent ordering
    std::vector<FileMetadata> sorted_metadata = metadata;
    std::sort(sorted_metadata.begin(), sorted_metadata.end(),
              [](const FileMetadata& a, const FileMetadata& b) {
                  return a.path < b.path;
              });
    
    for (const auto& meta : sorted_metadata) {
        json_array.push_back(metadata_to_json(meta));
    }

    // Create a temporary file first
    std::string temp_filepath = filepath + ".tmp";
    std::ofstream temp_file(temp_filepath);
    if (!temp_file) {
        throw std::runtime_error("Cannot create metadata file: " + temp_filepath);
    }
    
    temp_file << json_array.dump(4);
    temp_file.close();

    // Atomically rename the temporary file
    if (std::rename(temp_filepath.c_str(), filepath.c_str()) != 0) {
        throw std::runtime_error("Cannot save metadata file: " + filepath);
    }

    // Update backup history
    update_backup_history(filename, metadata);
}

void MetadataManager::update_backup_history(const std::string& metadata_file,
                                          const std::vector<FileMetadata>& metadata) {
    std::string history_file = metadata_dir_ + "/backup_history.json";
    nlohmann::json history = nlohmann::json::array();  // Always start with an empty array

    // Load existing history if it exists
    if (fs::exists(history_file)) {
        try {
            std::ifstream file(history_file);
            if (file) {
                nlohmann::json existing_history;
                file >> existing_history;
                if (existing_history.is_array()) {
                    history = existing_history;
                }
            }
        } catch (const nlohmann::json::exception&) {
            // If history file is corrupted, we already have an empty array
        }
    }

    // Create new backup entry
    nlohmann::json backup_entry;
    auto timestamp = get_timestamp_from_filename(metadata_file);
    backup_entry["timestamp"] = std::chrono::system_clock::to_time_t(timestamp);
    backup_entry["metadata_file"] = metadata_file;
    backup_entry["file_count"] = metadata.size();
    
    // Calculate total sizes
    std::uintmax_t total_original_size = 0;
    std::uintmax_t total_compressed_size = 0;
    size_t compressed_files = 0;
    
    for (const auto& meta : metadata) {
        if (!meta.is_directory) {
            total_original_size += meta.size;
            if (meta.is_compressed) {
                total_compressed_size += meta.compression_info.compressed_size;
                compressed_files++;
            } else {
                total_compressed_size += meta.size;
            }
        }
    }
    
    backup_entry["total_original_size"] = total_original_size;
    backup_entry["total_compressed_size"] = total_compressed_size;
    backup_entry["compressed_files"] = compressed_files;
    
    // Calculate overall compression ratio
    if (total_original_size == 0 || total_compressed_size == 0) {
        backup_entry["compression_ratio"] = 1.0;  // 1:1 ratio for empty backups
    } else {
        backup_entry["compression_ratio"] = 
            static_cast<double>(total_original_size) / total_compressed_size;
    }

    // Add to history
    history.push_back(backup_entry);

    // Save updated history
    std::string temp_history_file = history_file + ".tmp";
    std::ofstream temp_file(temp_history_file);
    if (!temp_file) {
        throw std::runtime_error("Cannot create history file: " + temp_history_file);
    }
    
    temp_file << history.dump(4);
    temp_file.close();

    // Atomically rename the temporary file
    if (std::rename(temp_history_file.c_str(), history_file.c_str()) != 0) {
        throw std::runtime_error("Cannot save history file: " + history_file);
    }
}

std::vector<FileMetadata> MetadataManager::load_latest_metadata() {
    auto history = get_backup_history();
    if (history.empty()) {
        return {};
    }
    return history.back().files;
}

std::vector<FileMetadata> MetadataManager::load_metadata(const std::string& metadata_file) {
    std::string filepath = metadata_dir_ + "/" + metadata_file;
    if (!fs::exists(filepath)) {
        throw std::runtime_error("Metadata file does not exist: " + filepath);
    }

    std::ifstream file(filepath);
    if (!file) {
        throw std::runtime_error("Failed to open metadata file for reading: " + filepath);
    }

    nlohmann::json json_array;
    file >> json_array;

    std::vector<FileMetadata> metadata_list;
    for (const auto& json : json_array) {
        metadata_list.push_back(json_to_metadata(json));
    }

    return metadata_list;
}

std::vector<BackupMetadata> MetadataManager::get_backup_history() {
    std::vector<BackupMetadata> history;
    std::string history_file = metadata_dir_ + "/backup_history.json";
    
    // Load backup history if it exists
    if (fs::exists(history_file)) {
        try {
            std::ifstream file(history_file);
            if (file) {
                nlohmann::json history_json;
                file >> history_json;
                
                // Make sure we have a JSON array
                if (!history_json.is_array()) {
                    throw std::runtime_error("Backup history is not in the correct format");
                }
                
                // Process each backup entry
                for (const auto& entry : history_json) {
                    BackupMetadata backup;
                    backup.metadata_file = entry["metadata_file"].get<std::string>();
                    backup.timestamp = std::chrono::system_clock::from_time_t(
                        entry["timestamp"].get<std::time_t>());
                    
                    // Load the actual metadata file
                    try {
                        backup.files = load_metadata(backup.metadata_file);
                        history.push_back(std::move(backup));
                    } catch (const std::exception& e) {
                        std::cerr << "Warning: Could not load metadata file " << backup.metadata_file 
                                << ": " << e.what() << std::endl;
                    }
                }
            }
        } catch (const std::exception& e) {
            std::cerr << "Warning: Could not load backup history: " << e.what() << std::endl;
            // Start with empty history if file is corrupted
        }
    }

    // If no valid history was loaded, scan metadata directory
    if (history.empty()) {
        for (const auto& entry : fs::directory_iterator(metadata_dir_)) {
            if (entry.path().extension() == ".json" && 
                entry.path().filename() != "backup_history.json") {
                try {
                    BackupMetadata backup;
                    backup.metadata_file = entry.path().filename().string();
                    backup.timestamp = get_timestamp_from_filename(backup.metadata_file);
                    backup.files = load_metadata(backup.metadata_file);
                    history.push_back(std::move(backup));
                } catch (const std::exception& e) {
                    std::cerr << "Warning: Could not load metadata file " 
                            << entry.path().filename().string() << ": " 
                            << e.what() << std::endl;
                }
            }
        }
    }

    // Sort by timestamp
    std::sort(history.begin(), history.end(),
              [](const BackupMetadata& a, const BackupMetadata& b) {
                  return a.timestamp < b.timestamp;
              });

    return history;
}

void MetadataManager::show_backup_status() {
    auto history = get_backup_history();
    
    if (history.empty()) {
        std::cout << "No backups have been performed yet." << std::endl;
        return;
    }

    std::cout << "\nBackup History:\n";
    for (const auto& backup : history) {
        auto time = std::chrono::system_clock::to_time_t(backup.timestamp);
        std::cout << "Backup from: " << std::put_time(std::localtime(&time), "%Y-%m-%d %H:%M:%S") << "\n"
                  << "Files: " << backup.files.size() << "\n"
                  << "Metadata file: " << backup.metadata_file << "\n"
                  << std::string(50, '-') << "\n";
    }
}

std::string MetadataManager::get_metadata_path() const {
    return metadata_dir_;
}

std::vector<FileMetadata> MetadataManager::get_changed_files(
    const std::vector<FileMetadata>& current_metadata,
    const std::vector<FileMetadata>& previous_metadata) {
    
    std::vector<FileMetadata> changed_files;
    std::unordered_map<std::string, FileMetadata> previous_map;

    // Create a map of previous metadata for quick lookup
    for (const auto& meta : previous_metadata) {
        previous_map[meta.path] = meta;
    }

    // Check each current file against previous metadata
    for (const auto& current : current_metadata) {
        auto it = previous_map.find(current.path);
        
        bool has_changed = false;
        if (it == previous_map.end()) {
            // New file
            has_changed = true;
        } else {
            // Check basic metadata changes
            has_changed = (current.checksum != it->second.checksum ||
                         current.size != it->second.size ||
                         current.last_modified != it->second.last_modified);

            // Check compression-related changes
            if (!has_changed && current.is_compressed && it->second.is_compressed) {
                has_changed = (current.compression_info.algorithm != it->second.compression_info.algorithm ||
                             current.compression_info.compression_level != it->second.compression_info.compression_level ||
                             current.compression_info.chunk_hashes != it->second.compression_info.chunk_hashes);
            } else if (current.is_compressed != it->second.is_compressed) {
                has_changed = true;
            }
        }

        if (has_changed) {
            changed_files.push_back(current);
        }
    }

    return changed_files;
}

nlohmann::json MetadataManager::metadata_to_json(const FileMetadata& metadata) {
    nlohmann::json json;
    json["path"] = metadata.path;
    json["size"] = metadata.size;
    json["last_modified"] = std::chrono::system_clock::to_time_t(metadata.last_modified);
    json["is_directory"] = metadata.is_directory;
    json["checksum"] = metadata.checksum;
    json["is_compressed"] = metadata.is_compressed;

    if (metadata.is_compressed) {
        nlohmann::json compression_info;
        compression_info["algorithm"] = metadata.compression_info.algorithm;
        compression_info["compression_level"] = metadata.compression_info.compression_level;
        compression_info["original_size"] = metadata.compression_info.original_size;
        compression_info["compressed_size"] = metadata.compression_info.compressed_size;
        
        // Handle empty files or zero compression ratio
        if (metadata.compression_info.original_size == 0 || metadata.compression_info.compressed_size == 0) {
            compression_info["compression_ratio"] = 1.0;  // 1:1 ratio for empty files
        } else {
            compression_info["compression_ratio"] = 
                static_cast<double>(metadata.compression_info.original_size) / 
                metadata.compression_info.compressed_size;
        }
        
        compression_info["chunk_hashes"] = metadata.compression_info.chunk_hashes;
        compression_info["chunk_sizes"] = metadata.compression_info.chunk_sizes;
        json["compression_info"] = compression_info;
    }

    return json;
}

FileMetadata MetadataManager::json_to_metadata(const nlohmann::json& json) {
    FileMetadata metadata;
    metadata.path = json["path"].get<std::string>();
    metadata.size = json["size"].get<std::uintmax_t>();
    metadata.last_modified = std::chrono::system_clock::from_time_t(
        json["last_modified"].get<std::time_t>());
    metadata.is_directory = json["is_directory"].get<bool>();
    metadata.checksum = json["checksum"].get<std::string>();
    metadata.is_compressed = json["is_compressed"].get<bool>();

    if (metadata.is_compressed && json.contains("compression_info")) {
        const auto& compression_info = json["compression_info"];
        metadata.compression_info.algorithm = compression_info["algorithm"].get<std::string>();
        metadata.compression_info.compression_level = compression_info["compression_level"].get<int>();
        metadata.compression_info.original_size = compression_info["original_size"].get<std::uintmax_t>();
        metadata.compression_info.compressed_size = compression_info["compressed_size"].get<std::uintmax_t>();
        metadata.compression_info.compression_ratio = compression_info["compression_ratio"].get<double>();
        metadata.compression_info.chunk_hashes = compression_info["chunk_hashes"].get<std::vector<std::string>>();
        metadata.compression_info.chunk_sizes = compression_info["chunk_sizes"].get<std::vector<std::size_t>>();
    }

    return metadata;
}

std::string MetadataManager::generate_metadata_filename() const {
    auto now = std::chrono::system_clock::now();
    auto time = std::chrono::system_clock::to_time_t(now);
    std::stringstream ss;
    ss << "backup_" << std::put_time(std::localtime(&time), "%Y%m%d_%H%M%S") << ".json";
    return ss.str();
}

std::chrono::system_clock::time_point MetadataManager::get_timestamp_from_filename(const std::string& filename) const {
    // Extract timestamp from filename (format: backup_YYYYMMDD_HHMMSS.json)
    std::string timestamp_str = filename.substr(7, 15); // Skip "backup_" and ".json"
    std::tm tm = {};
    std::stringstream ss(timestamp_str);
    ss >> std::get_time(&tm, "%Y%m%d_%H%M%S");
    return std::chrono::system_clock::from_time_t(std::mktime(&tm));
}

} // namespace backup
} // namespace hpe 