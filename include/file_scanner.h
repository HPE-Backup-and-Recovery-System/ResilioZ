#pragma once

#include <string>
#include <vector>
#include <filesystem>
#include <unordered_map>
#include <chrono>
#include <nlohmann/json.hpp>

namespace hpe {
namespace backup {

// Compression metadata following industry standards
struct CompressionMetadata {
    std::string algorithm;           // Compression algorithm (e.g., "zstd")
    int compression_level;           // Compression level used
    std::uintmax_t original_size;    // Original size before compression
    std::uintmax_t compressed_size;  // Size after compression
    double compression_ratio;        // Compression ratio achieved
    std::vector<std::string> chunk_hashes;  // SHA-256 hashes of compressed chunks
    std::vector<std::uintmax_t> chunk_sizes; // Sizes of compressed chunks
};

struct FileMetadata {
    std::string path;
    std::string checksum;
    std::uintmax_t size;
    std::chrono::system_clock::time_point last_modified;
    bool is_directory;
    bool is_compressed;
    CompressionMetadata compression_info;  // Compression-related metadata
};

class FileScanner {
public:
    FileScanner(const std::string& source_path);
    ~FileScanner() = default;

    // Scan the source directory and return metadata for all files
    std::vector<FileMetadata> scan_directory();
    
    // Calculate SHA-256 checksum for a file
    static std::string calculate_checksum(const std::string& file_path);
    
    // Get metadata for a single file
    FileMetadata get_file_metadata(const std::string& file_path);

private:
    std::string source_path_;
    std::unordered_map<std::string, FileMetadata> file_cache_;

    // Helper function to check if a file has changed
    bool has_file_changed(const std::string& file_path, const FileMetadata& previous_metadata);
};

} // namespace backup
} // namespace hpe 