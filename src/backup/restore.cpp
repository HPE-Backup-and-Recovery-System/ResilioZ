#include "backup/restore.hpp"
#include "backup/chunker.hpp"
#include "backup/progress.hpp"
#include <fstream>
#include <nlohmann/json.hpp>
#include <filesystem>
#include <iostream>
#include <zstd.h>
#include <iomanip>
#include <sstream>


Restore::Restore(const std::filesystem::path& input_path,
                 const std::filesystem::path& output_path,
                 const std::string& backup_name)
    : input_path_(input_path)
    , output_path_(output_path)
    , backup_name_(backup_name) {
    
    if (!std::filesystem::exists(input_path_)) {
        throw std::runtime_error("Input path does not exist: " + input_path_.string());
    }
    
    if (!std::filesystem::exists(output_path_)) {
        std::filesystem::create_directories(output_path_);
    }
    
    load_metadata();
}

void Restore::load_metadata() {
    std::filesystem::path metadata_path = input_path_ / "backup" / backup_name_;
    if (!std::filesystem::exists(metadata_path)) {
        throw std::runtime_error("Backup metadata not found: " + metadata_path.string());
    }
    
    std::ifstream metadata_file(metadata_path);
    nlohmann::json metadata_json;
    metadata_file >> metadata_json;
    
    metadata_.type = static_cast<BackupType>(metadata_json["type"].get<int>());
    metadata_.timestamp = std::chrono::system_clock::from_time_t(metadata_json["timestamp"].get<time_t>());
    metadata_.previous_backup = metadata_json["previous_backup"].get<std::string>();
    
    for (const auto& [file_path, file_json] : metadata_json["files"].items()) {
        FileMetadata file_metadata;
        file_metadata.original_filename = file_json["original_filename"];
        file_metadata.chunk_hashes = file_json["chunk_hashes"].get<std::vector<std::string>>();
        file_metadata.total_size = file_json["total_size"].get<uint64_t>();
        file_metadata.mtime = std::filesystem::file_time_type(
            std::chrono::duration_cast<std::filesystem::file_time_type::duration>(
                std::chrono::seconds(file_json["mtime"].get<time_t>())
            )
        );
        metadata_.files[file_path] = file_metadata;
    }
}

void Restore::restore_file(const std::string& filename) {
    auto file_metadata = find_file_metadata(filename);
    if (!file_metadata) {
        throw std::runtime_error("File not found in backup: " + filename);
    }
    
    std::filesystem::path output_file = prepare_output_path(filename, file_metadata->first);
    ProgressBar progress(file_metadata->second.total_size, file_metadata->second.chunk_hashes.size(),
                        "restore of " + filename);
    
    // Use streaming chunk combining
    chunker_.stream_combine_chunks(
        [&]() -> Chunk {
            return get_next_chunk(file_metadata->second, progress);
        },
        output_file,
        file_metadata->second.total_size
    );
    
    progress.complete();
    
    // Restore file metadata
    std::filesystem::last_write_time(output_file, file_metadata->second.mtime);
}

std::optional<std::pair<std::string, FileMetadata>> Restore::find_file_metadata(const std::string& filename) {
    auto it = std::find_if(metadata_.files.begin(), metadata_.files.end(),
        [&filename](const auto& pair) {
            return pair.second.original_filename == filename;
        });
    
    if (it == metadata_.files.end()) {
        return std::nullopt;
    }
    
    return std::make_pair(it->first, it->second);
}

std::filesystem::path Restore::prepare_output_path(const std::string& filename,
                                                 const std::string& original_path) {
    // Get the parent path from the original file path
    std::filesystem::path parent_path = std::filesystem::path(original_path).parent_path();
    
    // Create the parent directories inside the output path
    std::filesystem::path output_parent = output_path_;
    
    // Modify parent path to remove beginning /
    std::string path = parent_path.string();
    output_parent.append(path.substr(1,path.size()-1));

    // Create parent path in output path
    std::filesystem::create_directories(output_parent);
    
    // Create the final output path using the original filename
    return output_parent / filename;
}

Chunk Restore::get_next_chunk(const FileMetadata& file_metadata, ProgressBar& progress) {
    static size_t current_chunk = 0;
    static size_t processed_bytes = 0;
    
    if (current_chunk >= file_metadata.chunk_hashes.size()) {
        current_chunk = 0;
        processed_bytes = 0;
        return Chunk{};  // Return empty chunk to signal end
    }
    
    // Load and decompress the next chunk
    Chunk compressed_chunk = load_chunk(file_metadata.chunk_hashes[current_chunk]);
    Chunk decompressed_chunk = decompress_chunk(compressed_chunk);
    
    // Update progress
    processed_bytes += decompressed_chunk.size;
    current_chunk++;
    progress.update(processed_bytes, current_chunk);
    
    return decompressed_chunk;
}

void Restore::restore_all() {
    for (const auto& [file_path, metadata] : metadata_.files) {
        restore_file(metadata.original_filename);
    }
}

Chunk Restore::load_chunk(const std::string& hash) {
    // Use first two hex digits as subdirectory
    std::string subdir = hash.substr(0, 2);
    std::filesystem::path chunk_path = input_path_ / "chunks" / subdir / (hash + ".chunk");
    
    if (!std::filesystem::exists(chunk_path)) {
        throw std::runtime_error("Chunk not found: " + chunk_path.string());
    }
    
    std::ifstream chunk_file(chunk_path, std::ios::binary);
    if (!chunk_file) {
        throw std::runtime_error("Could not open chunk file: " + chunk_path.string());
    }
    
    Chunk chunk;
    chunk.hash = hash;
    chunk.data = std::vector<uint8_t>(
        (std::istreambuf_iterator<char>(chunk_file)),
        std::istreambuf_iterator<char>()
    );
    chunk.size = chunk.data.size();
    
    return chunk;
}

Chunk Restore::decompress_chunk(const Chunk& compressed_chunk) {
    // Get decompressed size from the first 8 bytes
    size_t decompressed_size = *reinterpret_cast<const size_t*>(compressed_chunk.data.data());
    
    // Create output buffer for decompressed data
    std::vector<uint8_t> decompressed_data(decompressed_size);
    
    // Decompress the data
    size_t const decompressed_bytes = ZSTD_decompress(
        decompressed_data.data(),
        decompressed_size,
        compressed_chunk.data.data() + sizeof(size_t),
        compressed_chunk.data.size() - sizeof(size_t)
    );
    
    if (ZSTD_isError(decompressed_bytes)) {
        throw std::runtime_error("Failed to decompress chunk: " + std::string(ZSTD_getErrorName(decompressed_bytes)));
    }
    
    Chunk decompressed_chunk;
    decompressed_chunk.hash = compressed_chunk.hash;
    decompressed_chunk.data = std::move(decompressed_data);
    decompressed_chunk.size = decompressed_bytes;
    
    return decompressed_chunk;
}

std::vector<std::string> Restore::list_backups() {
    std::vector<std::string> backups;
    for (const auto& entry : std::filesystem::directory_iterator(input_path_ / "backup")) {
        if (entry.is_regular_file()) {
            backups.push_back(entry.path().filename().string());
        }
    }
    std::sort(backups.begin(), backups.end());
    return backups;
}

void Restore::compare_backups(const std::string& backup1, const std::string& backup2) {
    // Load both backup metadata
    std::filesystem::path metadata_path1 = input_path_ / "backup" / backup1;
    std::filesystem::path metadata_path2 = input_path_ / "backup" / backup2;
    
    if (!std::filesystem::exists(metadata_path1) || !std::filesystem::exists(metadata_path2)) {
        throw std::runtime_error("One or both backup metadata files not found");
    }
    
    std::ifstream metadata_file1(metadata_path1);
    std::ifstream metadata_file2(metadata_path2);
    nlohmann::json metadata_json1, metadata_json2;
    metadata_file1 >> metadata_json1;
    metadata_file2 >> metadata_json2;
    
    BackupMetadata metadata1, metadata2;
    metadata1.type = static_cast<BackupType>(metadata_json1["type"].get<int>());
    metadata2.type = static_cast<BackupType>(metadata_json2["type"].get<int>());
    
    size_t changed_files = 0;
    size_t unchanged_files = 0;
    size_t added_files = 0;
    size_t deleted_files = 0;
    
    // Compare files
    for (const auto& [file_path, file_metadata2] : metadata_json2["files"].items()) {
        auto it = metadata_json1["files"].find(file_path);
        if (it == metadata_json1["files"].end()) {
            added_files++;
        } else if (file_metadata2["total_size"] != it.value()["total_size"] ||
                  file_metadata2["mtime"] != it.value()["mtime"]
                  ) {
            changed_files++;
        } else {
            unchanged_files++;
        }
    }
    
    // Count deleted files
    for (const auto& [file_path, _] : metadata_json1["files"].items()) {
        if (metadata_json2["files"].find(file_path) == metadata_json2["files"].end()) {
            deleted_files++;
        }
    }
    
    std::cout << "Backup Comparison (" << backup1 << " vs " << backup2 << "):\n"
              << "  Changed files: " << changed_files << "\n"
              << "  Unchanged files: " << unchanged_files << "\n"
              << "  Added files: " << added_files << "\n"
              << "  Deleted files: " << deleted_files << std::endl;
}

void Restore::restore(const std::filesystem::path& backup_path,
                     const std::filesystem::path& restore_path,
                     const std::string& backup_name) {
    // Load backup metadata
    input_path_ = backup_path;
    backup_name_ = backup_name;
    load_metadata();
    
    // Create restore directory if it doesn't exist
    std::filesystem::create_directories(restore_path);
    output_path_ = restore_path;
    
    // Process each file in the backup
    for (const auto& [file_path, file_metadata] : metadata_.files) {
        try {
            // Create the full restore path for this file
            auto file_restore_path = prepare_output_path(file_metadata.original_filename, file_path);
            
            // Create parent directories if they don't exist
            std::filesystem::create_directories(file_restore_path.parent_path());
            
            // Initialize progress tracking
            ProgressBar progress(file_metadata.total_size, file_metadata.chunk_hashes.size(),
                               "restore of " + file_metadata.original_filename);
            
            // Combine chunks into the restored file
            chunker_.stream_combine_chunks(
                [&]() -> Chunk {
                    return get_next_chunk(file_metadata, progress);
                },
                file_restore_path,
                file_metadata.total_size
            );
            
            // Set file timestamps
            std::filesystem::last_write_time(file_restore_path, file_metadata.mtime);
            
        } catch (const std::exception& e) {
            std::cerr << "Error restoring file " << file_path 
                      << ": " << e.what() << std::endl;
            throw;
        }
    }
}
