#pragma once

#include <string>
#include <vector>
#include <unordered_map>
#include <unordered_set>
#include <memory>
#include <filesystem>
#include <mutex>

namespace hpe::backup {

// Default extensions that are typically already compressed
const std::unordered_set<std::string> DEFAULT_COMPRESSED_EXTENSIONS = {
    ".zip", ".gz", ".bz2", ".xz", ".7z", ".rar",
    ".jpg", ".jpeg", ".png", ".gif", ".mp3", ".mp4",
    ".avi", ".mkv", ".mov", ".pdf"
};

struct CompressionStats {
    std::uintmax_t total_original_size{0};
    std::uintmax_t total_compressed_size{0};
    double average_compression_ratio{0.0};
    std::uintmax_t total_files_processed{0};
    std::uintmax_t total_chunks_processed{0};
    double average_chunk_size{0.0};
    std::chrono::microseconds total_compression_time{0};
};

/**
 * @class CompressionPolicy
 * @brief Manages compression decisions and statistics for the backup system.
 */
class CompressionPolicy {
public:
    struct Config {
        std::uintmax_t min_file_size;
        std::uintmax_t max_file_size;
        int default_level;
        int min_compression_ratio;
        bool compress_by_default;
        std::unordered_set<std::string> skip_extensions;
        std::unordered_map<std::string, int> extension_levels;

        Config()
            : min_file_size(4096)
            , max_file_size(1ULL << 40)
            , default_level(3)
            , min_compression_ratio(1)
            , compress_by_default(true)
            , skip_extensions(DEFAULT_COMPRESSED_EXTENSIONS)
        {}
    };

    explicit CompressionPolicy(Config config = Config{});
    
    // Delete copy constructor and assignment operator
    CompressionPolicy(const CompressionPolicy&) = delete;
    CompressionPolicy& operator=(const CompressionPolicy&) = delete;
    
    // Add move constructor and assignment operator
    CompressionPolicy(CompressionPolicy&& other) noexcept;
    CompressionPolicy& operator=(CompressionPolicy&& other) noexcept;

    /**
     * @brief Get the current configuration
     * @return Current configuration
     */
    Config get_config() const { return config_; }

    /**
     * @brief Determine if a file should be compressed
     * @param file_path Path to the file
     * @param file_size Size of the file
     * @return true if file should be compressed
     */
    bool should_compress(const std::filesystem::path& file_path, std::uintmax_t file_size) const;

    /**
     * @brief Get compression level for a file
     * @param file_path Path to the file
     * @return Compression level to use
     */
    int get_compression_level(const std::filesystem::path& file_path) const;

    /**
     * @brief Update compression statistics
     * @param original_size Original file size
     * @param compressed_size Compressed size
     * @param compression_time Time taken for compression
     * @param chunks_processed Number of chunks processed
     */
    void update_stats(std::uintmax_t original_size,
                     std::uintmax_t compressed_size,
                     std::chrono::microseconds compression_time,
                     size_t chunks_processed);

    /**
     * @brief Get current compression statistics
     * @return Current compression statistics
     */
    CompressionStats get_stats() const;

    /**
     * @brief Adjust compression policy based on statistics
     * This method analyzes compression statistics and adjusts the policy
     * for better performance/compression ratio balance
     */
    void optimize_policy();

    /**
     * @brief Reset compression statistics
     */
    void reset_stats();

private:
    Config config_;
    CompressionStats stats_;
    mutable std::mutex stats_mutex_;  // Protect stats in multi-threaded context

    /**
     * @brief Check if file extension indicates already compressed content
     * @param extension File extension
     * @return true if extension indicates compressed content
     */
    bool is_typically_compressed(const std::string& extension) const;

    /**
     * @brief Calculate optimal compression level based on file type and size
     * @param extension File extension
     * @param file_size File size
     * @return Optimal compression level
     */
    int calculate_optimal_level(const std::string& extension, std::uintmax_t file_size) const;
};

} // namespace hpe::backup 