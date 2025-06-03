#include "file_scanner.h"
#include <openssl/evp.h>
#include <fstream>
#include <sstream>
#include <iomanip>
#include <stdexcept>

namespace hpe {
namespace backup {

FileScanner::FileScanner(const std::string& source_path)
    : source_path_(source_path) {
    if (!std::filesystem::exists(source_path_)) {
        throw std::runtime_error("Source path does not exist: " + source_path_);
    }
}

std::vector<FileMetadata> FileScanner::scan_directory() {
    std::vector<FileMetadata> metadata_list;
    file_cache_.clear();

    for (const auto& entry : std::filesystem::recursive_directory_iterator(source_path_)) {
        FileMetadata metadata;
        metadata.path = entry.path().string();
        metadata.size = entry.file_size();
        
        // Convert file time to system time
        auto file_time = entry.last_write_time();
        auto sctp = std::chrono::time_point_cast<std::chrono::system_clock::duration>(
            file_time - std::filesystem::file_time_type::clock::now() + std::chrono::system_clock::now());
        metadata.last_modified = sctp;
        
        metadata.is_directory = entry.is_directory();

        if (!metadata.is_directory) {
            metadata.checksum = calculate_checksum(metadata.path);
        }

        file_cache_[metadata.path] = metadata;
        metadata_list.push_back(std::move(metadata));
    }

    return metadata_list;
}

std::string FileScanner::calculate_checksum(const std::string& file_path) {
    std::ifstream file(file_path, std::ios::binary);
    if (!file) {
        throw std::runtime_error("Failed to open file for checksum calculation: " + file_path);
    }

    // Use modern OpenSSL EVP interface
    EVP_MD_CTX* ctx = EVP_MD_CTX_new();
    if (!ctx) {
        throw std::runtime_error("Failed to create EVP context");
    }

    if (!EVP_DigestInit_ex(ctx, EVP_sha256(), nullptr)) {
        EVP_MD_CTX_free(ctx);
        throw std::runtime_error("Failed to initialize digest");
    }

    char buffer[4096];
    while (file.read(buffer, sizeof(buffer))) {
        if (!EVP_DigestUpdate(ctx, buffer, sizeof(buffer))) {
            EVP_MD_CTX_free(ctx);
            throw std::runtime_error("Failed to update digest");
        }
    }
    
    if (!EVP_DigestUpdate(ctx, buffer, file.gcount())) {
        EVP_MD_CTX_free(ctx);
        throw std::runtime_error("Failed to update digest");
    }

    unsigned char hash[EVP_MAX_MD_SIZE];
    unsigned int hash_len;
    if (!EVP_DigestFinal_ex(ctx, hash, &hash_len)) {
        EVP_MD_CTX_free(ctx);
        throw std::runtime_error("Failed to finalize digest");
    }

    EVP_MD_CTX_free(ctx);

    std::stringstream ss;
    for (unsigned int i = 0; i < hash_len; i++) {
        ss << std::hex << std::setw(2) << std::setfill('0') << static_cast<int>(hash[i]);
    }

    return ss.str();
}

FileMetadata FileScanner::get_file_metadata(const std::string& file_path) {
    if (file_cache_.find(file_path) != file_cache_.end()) {
        return file_cache_[file_path];
    }

    FileMetadata metadata;
    metadata.path = file_path;
    
    if (!std::filesystem::exists(file_path)) {
        throw std::runtime_error("File does not exist: " + file_path);
    }

    auto status = std::filesystem::status(file_path);
    metadata.is_directory = std::filesystem::is_directory(status);
    
    if (!metadata.is_directory) {
        metadata.size = std::filesystem::file_size(file_path);
        
        // Convert file time to system time
        auto file_time = std::filesystem::last_write_time(file_path);
        auto sctp = std::chrono::time_point_cast<std::chrono::system_clock::duration>(
            file_time - std::filesystem::file_time_type::clock::now() + std::chrono::system_clock::now());
        metadata.last_modified = sctp;
        
        metadata.checksum = calculate_checksum(file_path);
    }

    file_cache_[file_path] = metadata;
    return metadata;
}

bool FileScanner::has_file_changed(const std::string& file_path, const FileMetadata& previous_metadata) {
    try {
        FileMetadata current_metadata = get_file_metadata(file_path);
        
        if (current_metadata.size != previous_metadata.size) {
            return true;
        }

        if (current_metadata.last_modified != previous_metadata.last_modified) {
            return true;
        }

        if (current_metadata.checksum != previous_metadata.checksum) {
            return true;
        }

        return false;
    } catch (const std::exception& e) {
        // If we can't access the file, consider it changed
        return true;
    }
}

} // namespace backup
} // namespace hpe 