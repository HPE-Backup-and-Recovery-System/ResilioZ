#pragma once

#include <string>
#include <vector>
#include "file_scanner.h"
#include "metadata_manager.h"
#include "compression_handler.h"
#include "compression_policy.h"
#include "chunk_hasher.h"

namespace hpe {
namespace backup {

class UIHandler {
public:
    explicit UIHandler(const std::string& backup_path);
    
    void set_source_directory(const std::string& source_path);
    void run();

private:
    // Main menu functions
    void display_menu();
    int get_user_choice();
    void perform_backup();
    void show_backup_status();
    void show_changed_files();
    void configure_source_directory();
    void configure_compression_settings();

    // Compression settings submenu
    void display_compression_menu();
    void set_compression_level();
    void configure_file_type_settings();
    void set_compression_thresholds();
    void show_compression_stats();
    
    // Helper functions
    size_t calculate_total_size(const std::vector<FileMetadata>& files);
    void display_compression_progress(size_t processed_files, size_t total_files,
                                   const std::string& current_file,
                                   const CompressionStats& stats);
    void display_file_compression_info(const FileMetadata& file);
    std::string format_size(std::uintmax_t size_bytes);
    std::string format_compression_ratio(double ratio);

    // Member variables
    std::string backup_path_;
    MetadataManager metadata_manager_;
    std::string source_path_;
    CompressionPolicy compression_policy_;
    CompressionHandler compression_handler_;
    ChunkHasher chunk_hasher_;
};

} // namespace backup
} // namespace hpe 