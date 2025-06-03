#include "ui_handler.h"
#include <iostream>
#include <iomanip>
#include <ctime>
#include <filesystem>
#include <thread>
#include <chrono>
#include <unordered_map>
#include <fstream>
#include <vector>
#include <numeric>
#include <nlohmann/json.hpp>

namespace fs = std::filesystem;

namespace hpe {
namespace backup {

UIHandler::UIHandler(const std::string& backup_path)
    : backup_path_(backup_path),
      metadata_manager_(backup_path),
      source_path_("Not configured"),
      compression_policy_(),
      compression_handler_(),
      chunk_hasher_() {
}

void UIHandler::set_source_directory(const std::string& source_path) {
    if (!fs::exists(source_path)) {
        throw std::runtime_error("Source directory does not exist: " + source_path);
    }
    source_path_ = source_path;
}

void UIHandler::run() {
    while (true) {
        display_menu();
        int choice = get_user_choice();
        
        try {
            switch (choice) {
                case 1:
                    perform_backup();
                    break;
                case 2:
                    show_backup_status();
                    break;
                case 3:
                    show_changed_files();
                    break;
                case 4:
                    configure_source_directory();
                    break;
                case 5:
                    configure_compression_settings();
                    break;
                case 6:
                    std::cout << "Exiting program..." << std::endl;
                    return;
                default:
                    std::cout << "Invalid choice. Please try again." << std::endl;
            }
        } catch (const std::exception& e) {
            std::cerr << "Error: " << e.what() << std::endl;
        }
        
        std::cout << "\nPress Enter to continue...";
        std::cin.ignore(std::numeric_limits<std::streamsize>::max(), '\n');
    }
}

void UIHandler::display_menu() {
    std::cout << "\n=== Linux Incremental Backup System ===\n"
              << "Current source directory: " << source_path_ << "\n"
              << "Backup directory: " << backup_path_ << "\n\n"
              << "1. Perform Backup\n"
              << "2. Show Backup Status\n"
              << "3. Show Changed Files\n"
              << "4. Configure Source Directory\n"
              << "5. Compression Settings\n"
              << "6. Exit\n"
              << "Enter your choice: ";
}

int UIHandler::get_user_choice() {
    int choice;
    std::cin >> choice;
    std::cin.ignore(std::numeric_limits<std::streamsize>::max(), '\n');
    return choice;
}

void UIHandler::perform_backup() {
    if (source_path_ == "Not configured") {
        std::cout << "Please configure the source directory first (Option 4)." << std::endl;
        return;
    }

    std::cout << "Scanning files..." << std::endl;
    FileScanner scanner(source_path_);
    auto current_metadata = scanner.scan_directory();
    
    // Get previous metadata for comparison
    auto previous_metadata = metadata_manager_.load_latest_metadata();
    
    // Get changed files
    auto changed_files = metadata_manager_.get_changed_files(current_metadata, previous_metadata);
    
    if (changed_files.empty()) {
        std::cout << "No changes detected. Backup not needed." << std::endl;
        return;
    }

    std::cout << "Found " << changed_files.size() << " changed files." << std::endl;
    
    // Create a map to store updated metadata
    std::unordered_map<std::string, FileMetadata> updated_metadata;
    for (const auto& meta : current_metadata) {
        updated_metadata[meta.path] = meta;
    }
    
    // Create backup timestamp
    auto now = std::chrono::system_clock::now();
    auto time = std::chrono::system_clock::to_time_t(now);
    std::stringstream ss;
    ss << std::put_time(std::localtime(&time), "%Y%m%d_%H%M%S");
    std::string timestamp = ss.str();
    
    // Create backup directory for this snapshot
    fs::path backup_dir = fs::path(backup_path_) / "files" / timestamp;
    fs::create_directories(backup_dir);
    
    // Create a single archive for all changed files
    fs::path archive_path = backup_dir / "backup.zst";
    std::ofstream archive(archive_path, std::ios::binary);
    
    if (!archive) {
        throw std::runtime_error("Cannot create backup archive: " + archive_path.string());
    }
    
    // Process each changed file
    size_t processed = 0;
    std::vector<uint8_t> buffer(compression_handler_.DEFAULT_CHUNK_SIZE);
    std::uintmax_t total_original_size = 0;
    
    for (const auto& file : changed_files) {
        if (!file.is_directory) {
            auto start_time = std::chrono::steady_clock::now();
            total_original_size += file.size;
            
            // Get relative path for storing in archive
            fs::path relative_path = fs::relative(file.path, source_path_);
            
            // Write file metadata to archive
            nlohmann::json file_header;
            file_header["path"] = relative_path.string();
            file_header["size"] = file.size;
            std::string header = file_header.dump();
            uint32_t header_size = header.size();
            archive.write(reinterpret_cast<const char*>(&header_size), sizeof(header_size));
            archive.write(header.c_str(), header_size);
            
            // Open and read source file
            std::ifstream source(file.path, std::ios::binary);
            if (!source) {
                throw std::runtime_error("Cannot open source file: " + file.path);
            }
            
            // Initialize compression info
            FileMetadata& updated_file = updated_metadata[file.path];
            updated_file.is_compressed = true;
            updated_file.compression_info.algorithm = "zstd";
            updated_file.compression_info.compression_level = compression_handler_.getCompressionLevel();
            updated_file.compression_info.original_size = file.size;
            updated_file.compression_info.chunk_hashes.clear();
            updated_file.compression_info.chunk_sizes.clear();
            
            // Compress and write file content
            while (source) {
                source.read(reinterpret_cast<char*>(buffer.data()), buffer.size());
                std::streamsize bytes_read = source.gcount();
                
                if (bytes_read > 0) {
                    buffer.resize(bytes_read);
                    auto compressed_chunk = compression_handler_.compressChunk(buffer);
                    
                    // Write chunk size and data
                    uint32_t chunk_size = compressed_chunk.size();
                    archive.write(reinterpret_cast<const char*>(&chunk_size), sizeof(chunk_size));
                    archive.write(reinterpret_cast<const char*>(compressed_chunk.data()), chunk_size);
                    
                    // Calculate chunk hash
                    std::string chunk_hash = chunk_hasher_.calculate_hash(compressed_chunk);
                    
                    // Update chunk info
                    updated_file.compression_info.chunk_hashes.push_back(chunk_hash);
                    updated_file.compression_info.chunk_sizes.push_back(chunk_size);
                }
            }
            
            auto end_time = std::chrono::steady_clock::now();
            auto compression_time = std::chrono::duration_cast<std::chrono::microseconds>(
                end_time - start_time);
            
            // Update compression statistics
            std::uintmax_t total_compressed_size = std::accumulate(
                updated_file.compression_info.chunk_sizes.begin(),
                updated_file.compression_info.chunk_sizes.end(),
                0ULL);
            
            updated_file.compression_info.compressed_size = total_compressed_size;
            updated_file.compression_info.compression_ratio = 
                static_cast<double>(updated_file.compression_info.original_size) / total_compressed_size;
            
            compression_policy_.update_stats(
                file.size,
                total_compressed_size,
                compression_time,
                updated_file.compression_info.chunk_sizes.size()
            );
        }
        
        processed++;
        display_compression_progress(processed, changed_files.size(),
                                  file.path, compression_policy_.get_stats());
    }
    
    archive.close();
    
    // Calculate total compressed size
    std::uintmax_t total_compressed_size = fs::file_size(archive_path);
    
    // Update current_metadata with the changes
    current_metadata.clear();
    for (const auto& [path, meta] : updated_metadata) {
        current_metadata.push_back(meta);
    }
    
    // Save new metadata
    metadata_manager_.save_metadata(current_metadata);
    
    // Show final statistics
    auto stats = compression_policy_.get_stats();
    std::cout << "\nBackup completed successfully.\n"
              << "Files processed: " << stats.total_files_processed << "\n"
              << "Total original size: " << format_size(total_original_size) << "\n"
              << "Total compressed size: " << format_size(total_compressed_size) << "\n"
              << "Compression ratio: " << format_compression_ratio(
                     static_cast<double>(total_original_size) / total_compressed_size) << "\n"
              << "Space saved: " << format_size(total_original_size - total_compressed_size) << "\n"
              << "Backup location: " << archive_path.string() << "\n";
}

void UIHandler::show_backup_status() {
    auto history = metadata_manager_.get_backup_history();
    
    if (history.empty()) {
        std::cout << "No backups have been performed yet." << std::endl;
        return;
    }

    std::cout << "\nBackup History:\n";
    std::cout << std::setw(20) << "Timestamp" << " | "
              << std::setw(10) << "Files" << " | "
              << std::setw(15) << "Total Size" << " | "
              << std::setw(15) << "Compressed" << " | "
              << std::setw(15) << "Space Saved" << " | "
              << std::setw(10) << "Ratio" << "\n";
    std::cout << std::string(95, '-') << "\n";

    for (const auto& backup : history) {
        std::uintmax_t total_size = 0;
        std::uintmax_t compressed_size = 0;
        size_t compressed_files = 0;
        
        for (const auto& file : backup.files) {
            if (!file.is_directory) {
                total_size += file.size;
                if (file.is_compressed) {
                    compressed_size += file.compression_info.compressed_size;
                    compressed_files++;
                } else {
                    compressed_size += file.size;
                }
            }
        }

        auto time = std::chrono::system_clock::to_time_t(backup.timestamp);
        std::cout << std::put_time(std::localtime(&time), "%Y-%m-%d %H:%M:%S") << " | "
                  << std::setw(10) << backup.files.size() << " | "
                  << std::setw(15) << format_size(total_size) << " | "
                  << std::setw(15) << format_size(compressed_size) << " | "
                  << std::setw(15) << format_size(total_size - compressed_size) << " | "
                  << std::setw(10) << format_compression_ratio(static_cast<double>(total_size) / compressed_size) << "\n";
    }
}

void UIHandler::show_changed_files() {
    auto history = metadata_manager_.get_backup_history();
    
    if (history.size() < 2) {
        std::cout << "Need at least two backups to show changes." << std::endl;
        return;
    }

    const auto& current = history.back();
    const auto& previous = history[history.size() - 2];
    
    auto changed_files = metadata_manager_.get_changed_files(current.files, previous.files);
    
    if (changed_files.empty()) {
        std::cout << "No changes detected between the last two backups." << std::endl;
        return;
    }

    std::cout << "\nChanged Files:\n";
    std::cout << std::setw(50) << "Path" << " | "
              << std::setw(10) << "Size" << " | "
              << std::setw(10) << "Compressed" << " | "
              << std::setw(10) << "Ratio" << " | "
              << std::setw(20) << "Last Modified" << "\n";
    std::cout << std::string(105, '-') << "\n";

    for (const auto& file : changed_files) {
        display_file_compression_info(file);
    }
}

void UIHandler::configure_source_directory() {
    std::cout << "Enter the source directory path: ";
    std::string new_path;
    std::getline(std::cin, new_path);
    
    try {
        set_source_directory(new_path);
        std::cout << "Source directory configured successfully." << std::endl;
    } catch (const std::exception& e) {
        std::cerr << "Failed to configure source directory: " << e.what() << std::endl;
    }
}

void UIHandler::configure_compression_settings() {
    while (true) {
        display_compression_menu();
        int choice = get_user_choice();
        
        switch (choice) {
            case 1:
                set_compression_level();
                break;
            case 2:
                configure_file_type_settings();
                break;
            case 3:
                set_compression_thresholds();
                break;
            case 4:
                show_compression_stats();
                break;
            case 5:
                compression_policy_.reset_stats();
                std::cout << "Compression statistics reset." << std::endl;
                break;
            case 6:
                return;
            default:
                std::cout << "Invalid choice. Please try again." << std::endl;
        }
    }
}

void UIHandler::display_compression_menu() {
    std::cout << "\n=== Compression Settings ===\n"
              << "1. Set Default Compression Level\n"
              << "2. Configure File Type Settings\n"
              << "3. Set Size Thresholds\n"
              << "4. View Compression Statistics\n"
              << "5. Reset Statistics\n"
              << "6. Back to Main Menu\n"
              << "Enter your choice: ";
}

void UIHandler::set_compression_level() {
    std::cout << "Enter new default compression level (1-22): ";
    int level;
    std::cin >> level;
    std::cin.ignore(std::numeric_limits<std::streamsize>::max(), '\n');
    
    try {
        if (level < 1 || level > 22) {
            throw std::runtime_error("Compression level must be between 1 and 22");
        }
        
        // Update both policy and handler
        CompressionPolicy::Config config = compression_policy_.get_config();
        config.default_level = level;
        compression_policy_ = CompressionPolicy(config);
        compression_handler_.setCompressionLevel(level);
        
        std::cout << "Default compression level updated to " << level << std::endl;
    } catch (const std::exception& e) {
        std::cerr << "Error: " << e.what() << std::endl;
    }
}

void UIHandler::configure_file_type_settings() {
    std::cout << "Enter file extension (e.g., .txt): ";
    std::string ext;
    std::getline(std::cin, ext);
    
    // Convert to lowercase for consistency
    std::transform(ext.begin(), ext.end(), ext.begin(), ::tolower);
    
    // Make sure extension starts with a dot
    if (!ext.empty() && ext[0] != '.') {
        ext = "." + ext;
    }
    
    std::cout << "Enter compression level for " << ext << " (1-22): ";
    int level;
    std::cin >> level;
    std::cin.ignore(std::numeric_limits<std::streamsize>::max(), '\n');
    
    try {
        if (level < 1 || level > 22) {
            throw std::runtime_error("Compression level must be between 1 and 22");
        }
        
        CompressionPolicy::Config config = compression_policy_.get_config();
        config.extension_levels[ext] = level;
        compression_policy_ = CompressionPolicy(config);
        
        // Update handler's extension-specific settings
        compression_handler_.setExtensionCompressionLevel(ext, level);
        
        std::cout << "File type compression settings updated for " << ext << " to level " << level << std::endl;
    } catch (const std::exception& e) {
        std::cerr << "Error: " << e.what() << std::endl;
    }
}

void UIHandler::set_compression_thresholds() {
    std::cout << "Enter minimum file size for compression (bytes): ";
    std::uintmax_t min_size;
    std::cin >> min_size;
    
    std::cout << "Enter maximum file size for compression (bytes): ";
    std::uintmax_t max_size;
    std::cin >> max_size;
    
    std::cin.ignore(std::numeric_limits<std::streamsize>::max(), '\n');
    
    try {
        if (min_size >= max_size) {
            throw std::runtime_error("Minimum size must be less than maximum size");
        }
        
        CompressionPolicy::Config config = compression_policy_.get_config();
        config.min_file_size = min_size;
        config.max_file_size = max_size;
        compression_policy_ = CompressionPolicy(config);
        
        // Update handler's thresholds
        compression_handler_.setCompressionThresholds(min_size, max_size);
        
        std::cout << "Compression size thresholds updated:\n"
                  << "Minimum size: " << format_size(min_size) << "\n"
                  << "Maximum size: " << format_size(max_size) << std::endl;
    } catch (const std::exception& e) {
        std::cerr << "Error: " << e.what() << std::endl;
    }
}

void UIHandler::show_compression_stats() {
    auto stats = compression_policy_.get_stats();
    
    if (stats.total_files_processed == 0) {
        std::cout << "\nNo compression statistics available yet. Perform a backup to generate statistics.\n";
        return;
    }
    
    std::cout << "\n=== Compression Statistics ===\n"
              << "Total files processed: " << stats.total_files_processed << "\n"
              << "Total original size: " << format_size(stats.total_original_size) << "\n"
              << "Total compressed size: " << format_size(stats.total_compressed_size) << "\n"
              << "Space saved: " << format_size(stats.total_original_size - stats.total_compressed_size) << "\n"
              << "Average compression ratio: " << format_compression_ratio(stats.average_compression_ratio) << "\n"
              << "Average chunk size: " << format_size(static_cast<std::uintmax_t>(stats.average_chunk_size)) << "\n"
              << "Total compression time: " << stats.total_compression_time.count() / 1000000.0 << " seconds\n";
    
    // Show current settings
    auto config = compression_policy_.get_config();
    std::cout << "\nCurrent Settings:\n"
              << "Default compression level: " << config.default_level << "\n"
              << "Minimum file size: " << format_size(config.min_file_size) << "\n"
              << "Maximum file size: " << format_size(config.max_file_size) << "\n"
              << "File type specific settings:\n";
    
    for (const auto& [ext, level] : config.extension_levels) {
        std::cout << "  " << ext << ": Level " << level << "\n";
    }
}

void UIHandler::display_compression_progress(size_t processed_files, size_t total_files,
                                          const std::string& current_file,
                                          const CompressionStats& stats) {
    const int bar_width = 50;
    float progress = static_cast<float>(processed_files) / total_files;
    int pos = static_cast<int>(bar_width * progress);
    
    std::cout << "\r[";
    for (int i = 0; i < bar_width; ++i) {
        if (i < pos) std::cout << "=";
        else if (i == pos) std::cout << ">";
        else std::cout << " ";
    }
    std::cout << "] " << int(progress * 100.0) << "% "
              << processed_files << "/" << total_files << " | "
              << "Current: " << fs::path(current_file).filename().string() << " | "
              << "Ratio: " << format_compression_ratio(stats.average_compression_ratio)
              << std::flush;
}

void UIHandler::display_file_compression_info(const FileMetadata& file) {
    auto time = std::chrono::system_clock::to_time_t(file.last_modified);
    std::cout << std::setw(50) << file.path << " | "
              << std::setw(10) << format_size(file.size) << " | "
              << std::setw(10) << (file.is_compressed ? "Yes" : "No") << " | "
              << std::setw(10) << (file.is_compressed ? 
                                  format_compression_ratio(file.compression_info.compression_ratio) : 
                                  "N/A") << " | "
              << std::put_time(std::localtime(&time), "%Y-%m-%d %H:%M:%S") << "\n";
}

std::string UIHandler::format_size(std::uintmax_t size_bytes) {
    const char* units[] = {"B", "KB", "MB", "GB", "TB"};
    int unit_index = 0;
    double size = static_cast<double>(size_bytes);
    
    while (size >= 1024.0 && unit_index < 4) {
        size /= 1024.0;
        unit_index++;
    }
    
    std::stringstream ss;
    ss << std::fixed << std::setprecision(2) << size << units[unit_index];
    return ss.str();
}

std::string UIHandler::format_compression_ratio(double ratio) {
    std::stringstream ss;
    ss << std::fixed << std::setprecision(2) << ratio << "x";
    return ss.str();
}

} // namespace backup
} // namespace hpe 