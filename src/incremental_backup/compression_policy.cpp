#include "compression_policy.h"
#include <algorithm>
#include <cmath>
#include <mutex>

namespace hpe::backup {

CompressionPolicy::CompressionPolicy(Config config)
    : config_(std::move(config)) {
    // Initialize default extension-specific compression levels
    if (config_.extension_levels.empty()) {
        // Text files - higher compression
        config_.extension_levels[".txt"] = 9;
        config_.extension_levels[".log"] = 9;
        config_.extension_levels[".xml"] = 9;
        config_.extension_levels[".json"] = 9;
        config_.extension_levels[".yml"] = 9;
        config_.extension_levels[".md"] = 9;
        
        // Source code - higher compression
        config_.extension_levels[".cpp"] = 9;
        config_.extension_levels[".h"] = 9;
        config_.extension_levels[".py"] = 9;
        config_.extension_levels[".java"] = 9;
        
        // Binary files - lower compression
        config_.extension_levels[".bin"] = 3;
        config_.extension_levels[".exe"] = 3;
        config_.extension_levels[".dll"] = 3;
        config_.extension_levels[".so"] = 3;
        
        // Database files - medium compression
        config_.extension_levels[".db"] = 5;
        config_.extension_levels[".sqlite"] = 5;
    }
}

CompressionPolicy::CompressionPolicy(CompressionPolicy&& other) noexcept
    : config_(std::move(other.config_))
    , stats_(std::move(other.stats_)) {}

CompressionPolicy& CompressionPolicy::operator=(CompressionPolicy&& other) noexcept {
    if (this != &other) {
        config_ = std::move(other.config_);
        stats_ = std::move(other.stats_);
    }
    return *this;
}

bool CompressionPolicy::should_compress(const std::filesystem::path& file_path, std::uintmax_t file_size) const {
    // Check file size constraints
    if (file_size < config_.min_file_size || file_size > config_.max_file_size) {
        return false;
    }

    std::string extension = file_path.extension().string();
    std::transform(extension.begin(), extension.end(), extension.begin(), ::tolower);

    // Skip already compressed files
    if (is_typically_compressed(extension)) {
        return false;
    }

    return config_.compress_by_default;
}

int CompressionPolicy::get_compression_level(const std::filesystem::path& file_path) const {
    std::string extension = file_path.extension().string();
    std::transform(extension.begin(), extension.end(), extension.begin(), ::tolower);

    auto it = config_.extension_levels.find(extension);
    if (it != config_.extension_levels.end()) {
        return it->second;
    }

    return config_.default_level;
}

void CompressionPolicy::update_stats(
    std::uintmax_t original_size,
    std::uintmax_t compressed_size,
    std::chrono::microseconds compression_time,
    size_t chunks_processed) {
    
    std::lock_guard<std::mutex> lock(stats_mutex_);
    
    stats_.total_original_size += original_size;
    stats_.total_compressed_size += compressed_size;
    stats_.total_files_processed++;
    stats_.total_chunks_processed += chunks_processed;
    stats_.total_compression_time += compression_time;
    
    // Update averages
    stats_.average_compression_ratio = static_cast<double>(stats_.total_original_size) /
                                     static_cast<double>(stats_.total_compressed_size);
    
    stats_.average_chunk_size = static_cast<double>(stats_.total_original_size) /
                               static_cast<double>(stats_.total_chunks_processed);
}

CompressionStats CompressionPolicy::get_stats() const {
    std::lock_guard<std::mutex> lock(stats_mutex_);
    return stats_;
}

void CompressionPolicy::optimize_policy() {
    std::lock_guard<std::mutex> lock(stats_mutex_);
    
    if (stats_.total_files_processed < 100) {
        // Not enough data to make informed decisions
        return;
    }

    // Adjust compression levels based on compression ratio and time
    double compression_speed = static_cast<double>(stats_.total_original_size) /
                             stats_.total_compression_time.count();
    
    // If compression is too slow (less than 100MB/s) and ratio is not great (< 2.0),
    // reduce default compression level
    if (compression_speed < 100 * 1024 * 1024 && stats_.average_compression_ratio < 2.0) {
        config_.default_level = std::max(1, config_.default_level - 1);
    }
    
    // If compression is fast (more than 500MB/s) and ratio could be better (< 3.0),
    // increase default compression level
    if (compression_speed > 500 * 1024 * 1024 && stats_.average_compression_ratio < 3.0) {
        config_.default_level = std::min(22, config_.default_level + 1);
    }

    // Adjust minimum file size based on compression ratio
    if (stats_.average_compression_ratio < 1.2) {
        config_.min_file_size *= 2;  // Increase minimum size if compression is not effective
    }
}

void CompressionPolicy::reset_stats() {
    std::lock_guard<std::mutex> lock(stats_mutex_);
    stats_ = CompressionStats{};
}

bool CompressionPolicy::is_typically_compressed(const std::string& extension) const {
    return config_.skip_extensions.find(extension) != config_.skip_extensions.end();
}

int CompressionPolicy::calculate_optimal_level(const std::string& extension, std::uintmax_t file_size) const {
    // Start with the extension-specific level or default
    int level = get_compression_level(std::filesystem::path("dummy" + extension));
    
    // Adjust based on file size
    if (file_size < 1024 * 1024) {  // < 1MB
        level = std::min(level, 3);  // Use faster compression for small files
    } else if (file_size > 1024 * 1024 * 1024) {  // > 1GB
        level = std::min(level, 5);  // Limit compression level for very large files
    }
    
    return level;
}

} // namespace hpe::backup 