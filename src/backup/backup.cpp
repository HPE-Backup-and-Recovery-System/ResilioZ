#include "backup/backup.hpp"
#include "backup/progress.hpp"
#include <fstream>
#include <nlohmann/json.hpp>
#include <openssl/sha.h>
#include <filesystem>
#include <iostream>
#include <zstd.h>
#include <iomanip>
#include <sstream>
#include <algorithm>
#include <set>
#include <map>

Backup::Backup(const std::filesystem::path& input_path,
               const std::filesystem::path& output_path,
               BackupType type,
               const std::string& remarks,
               size_t average_chunk_size)
    : input_path_(input_path)
    , output_path_(output_path)
    , chunker_(average_chunk_size)
    , backup_type_(type) {
    
    if (!std::filesystem::exists(input_path_)) {
        throw std::runtime_error("Input path does not exist: " + input_path_.string());
    }
    
    // Create necessary directories
    std::filesystem::create_directories(output_path_ / "backup");
    std::filesystem::create_directories(output_path_ / "chunks");
    
    // Initialize metadata
    metadata_.type = type;
    metadata_.timestamp = std::chrono::system_clock::now();
    metadata_.remarks = remarks;
    
    // For incremental/differential backups, load previous metadata
    if (type != BackupType::FULL) {
        std::string previous_backup;
        if (type == BackupType::INCREMENTAL) {
            previous_backup = get_latest_backup(output_path_);
        } else { // DIFFERENTIAL
            previous_backup = get_latest_full_backup(output_path_);
        }
        
        if (previous_backup.empty()) {
            std::string backup_type = (type == BackupType::INCREMENTAL) ? "incremental" : "differential";
            throw std::runtime_error("No suitable previous backup found for " + backup_type + " backup. Please perform a full backup first.");
        }
        
        metadata_.previous_backup = previous_backup;
        BackupMetadata prev_metadata = load_previous_metadata(output_path_, previous_backup);
        metadata_.files = prev_metadata.files;
    }
}

void Backup::backup_file(const std::filesystem::path& file_path) {
    if (should_skip_file(file_path)) {
        return;
    }
    
    auto file_metadata = create_file_metadata(file_path);
    ProgressBar progress(file_metadata.total_size, 0, "backup of " + file_path.string());
    
    size_t processed_bytes = 0;
    size_t processed_chunks = 0;
    
    // Use streaming chunking
    chunker_.stream_split_file(file_path, [&](const Chunk& chunk) {
        // Compress the chunk before saving
        Chunk compressed_chunk = compress_chunk(chunk);
        file_metadata.chunk_hashes.push_back(compressed_chunk.hash);
        save_chunk(compressed_chunk);
        
        // Update progress
        processed_bytes += chunk.size;
        processed_chunks++;
        progress.update(processed_bytes, processed_chunks);
    });
    
    progress.complete();
    
    metadata_.files[file_path.string()] = file_metadata;

}

bool Backup::should_skip_file(const std::filesystem::path& file_path) {
    if (backup_type_ != BackupType::FULL) {
        auto it = metadata_.files.find(file_path.string());
        if (it != metadata_.files.end() && !has_file_changed(file_path, it->second)) {
            std::cout << "Skipping unchanged file: " << file_path << std::endl;
            return true;
        }
    }
    return false;
}

FileMetadata Backup::create_file_metadata(const std::filesystem::path& file_path) {
    FileMetadata metadata;
    metadata.original_filename = file_path.filename().string();
    metadata.total_size = std::filesystem::file_size(file_path);
    metadata.mtime = std::filesystem::last_write_time(file_path);
    return metadata;
}

void Backup::process_chunk(const Chunk& chunk, FileMetadata& file_metadata, ProgressBar& progress) {
    // Compress the chunk before saving
    Chunk compressed_chunk = compress_chunk(chunk);
    file_metadata.chunk_hashes.push_back(compressed_chunk.hash);
    save_chunk(compressed_chunk);
    
    // Update progress
    progress.update(chunk.size, file_metadata.chunk_hashes.size());
}

void Backup::backup_directory() {
    size_t changed_files = 0;
    size_t unchanged_files = 0;
    size_t added_files = 0;
    size_t deleted_files = 0;
    
    // Track files in current backup
    std::set<std::string> current_files;
    
    // First, check for deleted files

    for (const auto& [file_path, _] : metadata_.files) {
        if (!std::filesystem::exists(file_path)) {
            deleted_files++;
            metadata_.files.erase(file_path);
        }
    }

    // Then process existing files
    for (const auto& entry : std::filesystem::recursive_directory_iterator(input_path_)) {
        if (entry.is_regular_file()) {
            std::string file_path = entry.path().string();
            current_files.insert(file_path);
            
            auto it = metadata_.files.find(file_path);
            if (it == metadata_.files.end()) {
                added_files++;
                backup_file(entry.path());
            } else if (has_file_changed(entry.path(), it->second)) {
                changed_files++;
                backup_file(entry.path());
            } else {
                unchanged_files++;
            }
        }
    }
    
    std::cout << "Backup Summary:\n"
              << "  Changed files: " << changed_files << "\n"
              << "  Unchanged files: " << unchanged_files << "\n"
              << "  Added files: " << added_files << "\n"
              << "  Deleted files: " << deleted_files << std::endl;
    
    save_metadata();
}

void Backup::save_metadata() {
    // Generate backup name from timestamp
    auto time = std::chrono::system_clock::to_time_t(metadata_.timestamp);
    std::stringstream ss;
    ss << std::put_time(std::localtime(&time), "%Y%m%d_%H%M%S");
    std::string backup_name = ss.str();
    
    // Convert metadata to JSON
    nlohmann::json metadata_json;
    metadata_json["type"] = static_cast<int>(metadata_.type);
    metadata_json["timestamp"] = std::chrono::system_clock::to_time_t(metadata_.timestamp);
    metadata_json["previous_backup"] = metadata_.previous_backup;
    metadata_json["remarks"] = metadata_.remarks;
    
    nlohmann::json files_json;
    for (const auto& [file_path, file_metadata] : metadata_.files) {
        nlohmann::json file_json;
        file_json["original_filename"] = file_metadata.original_filename;
        file_json["chunk_hashes"] = file_metadata.chunk_hashes;
        file_json["total_size"] = file_metadata.total_size;
        
        // Convert file times to seconds since epoch
        auto mtime_seconds = std::chrono::duration_cast<std::chrono::seconds>(
            file_metadata.mtime.time_since_epoch()
        ).count();
        
        file_json["mtime"] = mtime_seconds;
        files_json[file_path] = file_json;
    }
    metadata_json["files"] = files_json;
    
    // Save metadata
    std::ofstream metadata_file(output_path_ / "backup" / backup_name);
    metadata_file << metadata_json.dump(4);
}

std::string Backup::generate_chunk_filename(const std::string& hash) {
    // Use first two hex digits as subdirectory
    std::string subdir = hash.substr(0, 2);
    std::filesystem::create_directories(output_path_ / "chunks" / subdir);
    return subdir + "/" + hash + ".chunk";
}

Chunk Backup::compress_chunk(const Chunk& original_chunk) {
    // Calculate maximum compressed size
    size_t const max_compressed_size = ZSTD_compressBound(original_chunk.size);
    
    // Create buffer for compressed data (including size prefix)
    std::vector<uint8_t> compressed_data(sizeof(size_t) + max_compressed_size);
    
    // Store original size at the beginning
    *reinterpret_cast<size_t*>(compressed_data.data()) = original_chunk.size;
    
    // Compress the data
    size_t const compressed_bytes = ZSTD_compress(
        compressed_data.data() + sizeof(size_t),
        max_compressed_size,
        original_chunk.data.data(),
        original_chunk.size,
        ZSTD_CLEVEL_DEFAULT
    );
    
    if (ZSTD_isError(compressed_bytes)) {
        throw std::runtime_error("Failed to compress chunk: " + std::string(ZSTD_getErrorName(compressed_bytes)));
    }
    
    // Resize the vector to actual compressed size + size prefix
    compressed_data.resize(sizeof(size_t) + compressed_bytes);
    
    // Calculate new hash for compressed chunk
    unsigned char hash[SHA256_DIGEST_LENGTH];
    SHA256_CTX sha256;
    SHA256_Init(&sha256);
    SHA256_Update(&sha256, compressed_data.data(), compressed_data.size());
    SHA256_Final(hash, &sha256);
    
    // Convert hash to hex string
    std::stringstream ss;
    for (int i = 0; i < SHA256_DIGEST_LENGTH; i++) {
        ss << std::hex << std::setw(2) << std::setfill('0') << static_cast<int>(hash[i]);
    }
    
    Chunk compressed_chunk;
    compressed_chunk.hash = ss.str();
    compressed_chunk.data = std::move(compressed_data);
    compressed_chunk.size = compressed_bytes;  // Store actual compressed size
    return compressed_chunk;
}

void Backup::save_chunk(const Chunk& chunk) {
    std::filesystem::path chunk_path = output_path_ / "chunks" / generate_chunk_filename(chunk.hash);
    
    if (!std::filesystem::exists(chunk_path)) {
        std::ofstream chunk_file(chunk_path, std::ios::binary);
        if (!chunk_file) {
            throw std::runtime_error("Could not create chunk file: " + chunk_path.string());
        }
        chunk_file.write(reinterpret_cast<const char*>(chunk.data.data()), chunk.data.size());
    }
}

BackupMetadata Backup::load_previous_metadata(const std::filesystem::path& backup_dir,
                                            const std::string& backup_name) {
    std::filesystem::path metadata_path = backup_dir / "backup" / backup_name;
    if (!std::filesystem::exists(metadata_path)) {
        throw std::runtime_error("Previous backup metadata not found: " + metadata_path.string());
    }
    
    std::ifstream metadata_file(metadata_path);
    nlohmann::json metadata_json;
    metadata_file >> metadata_json;
    
    BackupMetadata metadata;
    metadata.type = static_cast<BackupType>(metadata_json["type"].get<int>());
    metadata.timestamp = std::chrono::system_clock::from_time_t(metadata_json["timestamp"].get<time_t>());
    metadata.previous_backup = metadata_json["previous_backup"].get<std::string>();
    metadata.remarks = metadata_json.value("remarks", "");
    
    for (const auto& [file_path, file_json] : metadata_json["files"].items()) {
        FileMetadata file_metadata;
        file_metadata.original_filename = file_json["original_filename"];
        file_metadata.chunk_hashes = file_json["chunk_hashes"].get<std::vector<std::string>>();
        file_metadata.total_size = file_json["total_size"];
        file_metadata.mtime = std::filesystem::file_time_type(
            std::chrono::duration_cast<std::filesystem::file_time_type::duration>(
                std::chrono::seconds(file_json["mtime"].get<time_t>())
            )
        );

        metadata.files[file_path] = file_metadata;
    }
    
    return metadata;
}

std::string Backup::get_latest_backup(const std::filesystem::path& backup_dir) {
    auto backups = list_backups(backup_dir);
    return backups.empty() ? "" : backups[0];
}

std::string Backup::get_latest_full_backup(const std::filesystem::path& backup_dir) {
    auto backups = list_backups(backup_dir);
    for (auto it = backups.begin(); it != backups.end(); ++it) {
        std::filesystem::path metadata_path = backup_dir / "backup" / *it;
        std::ifstream metadata_file(metadata_path);
        nlohmann::json metadata_json;
        metadata_file >> metadata_json;
        if (static_cast<BackupType>(metadata_json["type"].get<int>()) == BackupType::FULL) {
            return *it;
        }
    }
    return "";
}

std::vector<std::string> Backup::list_backups(const std::filesystem::path& backup_dir) {
    std::vector<std::string> backups;
    for (const auto& entry : std::filesystem::directory_iterator(backup_dir / "backup")) {
        if (entry.is_regular_file()) {
            backups.push_back(entry.path().filename().string());
        }
    }

    // Return backups in descending order
    std::sort(backups.rbegin(), backups.rend());

    return backups;
}

void Backup::print_all_backup_details(const std::filesystem::path& backup_dir) {
    auto backups = list_backups(backup_dir);
    

    // Print backup details
    std::cout << "\nBackup List:\n";
    std::cout << std::setw(20) << "Backup Name" << " | "
              << std::setw(10) << "Type" << " | "
              << std::setw(20) << "Time" << " | "
              << std::setw(30) << "Remarks" << "\n";
    std::cout << std::string(90, '-') << "\n";

    for (const auto& backup : backups) {
        std::filesystem::path metadata_path = backup_dir / "backup" / backup;
        std::ifstream metadata_file(metadata_path);
        nlohmann::json metadata_json;
        metadata_file >> metadata_json;

        auto type = static_cast<BackupType>(metadata_json["type"].get<int>());
        auto timestamp = std::chrono::system_clock::from_time_t(metadata_json["timestamp"].get<time_t>());
        auto time = std::chrono::system_clock::to_time_t(timestamp);
        std::stringstream ss;
        ss << std::put_time(std::localtime(&time), "%Y-%m-%d %H:%M:%S");

        std::string type_str;
        switch (type) {
            case BackupType::FULL: type_str = "FULL"; break;
            case BackupType::INCREMENTAL: type_str = "INCREMENTAL"; break;
            case BackupType::DIFFERENTIAL: type_str = "DIFFERENTIAL"; break;
        }

        std::string remarks = metadata_json.value("remarks", "");
        if (remarks.length() > 27) {
            remarks = remarks.substr(0, 24) + "...";
        }

        std::cout << std::setw(20) << backup << " | "
                  << std::setw(10) << type_str << " | "
                  << std::setw(20) << ss.str() << " | "
                  << std::setw(30) << remarks << "\n";
    }
    std::cout << std::string(90, '-') << "\n\n";
} 

void Backup::compare_backups(const std::filesystem::path& backup_dir,
                           const std::string& backup1,
                           const std::string& backup2) {
    BackupMetadata metadata1 = load_previous_metadata(backup_dir, backup1);
    BackupMetadata metadata2 = load_previous_metadata(backup_dir, backup2);
    
    size_t changed_files = 0;
    size_t unchanged_files = 0;
    size_t added_files = 0;
    size_t deleted_files = 0;
    
    // Compare files
    for (const auto& [file_path, file_metadata2] : metadata2.files) {
        auto it = metadata1.files.find(file_path);
        if (it == metadata1.files.end()) {
            added_files++;
        } else if (file_metadata2.total_size != it->second.total_size ||
                  file_metadata2.mtime != it->second.mtime ) {
            changed_files++;
        } else {
            unchanged_files++;
        }
    }
    
    // Count deleted files
    for (const auto& [file_path, _] : metadata1.files) {
        if (metadata2.files.find(file_path) == metadata2.files.end()) {
            deleted_files++;
        }
    }
    
    std::cout << "Backup Comparison (" << backup1 << " vs " << backup2 << "):\n"
              << "  Changed files: " << changed_files << "\n"
              << "  Unchanged files: " << unchanged_files << "\n"
              << "  Added files: " << added_files << "\n"
              << "  Deleted files: " << deleted_files << std::endl;
}

bool Backup::has_file_changed(const std::filesystem::path& file_path,
                            const FileMetadata& previous_metadata) {
    auto file_status = std::filesystem::status(file_path);
    auto mtime = std::filesystem::last_write_time(file_path);
    auto size = std::filesystem::file_size(file_path);
    
    // Convert both times to seconds for comparison
    auto current_mtime_seconds = std::chrono::duration_cast<std::chrono::seconds>(
        mtime.time_since_epoch()
    ).count();
    auto previous_mtime_seconds = std::chrono::duration_cast<std::chrono::seconds>(
        previous_metadata.mtime.time_since_epoch()
    ).count();
    


    
    return size != previous_metadata.total_size ||
           current_mtime_seconds != previous_mtime_seconds;
}

