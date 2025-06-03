#pragma once

#include <string>
#include <vector>
#include <memory>
#include <cstdint>
#include <unordered_map>
#include <zstd.h>

namespace hpe {

/**
 * @brief Class to handle ZSTD compression operations
 * 
 * This class provides functionality for compressing and decompressing data
 * using Facebook's ZSTD compression algorithm. It handles chunking of data
 * and maintains compression statistics.
 */
class CompressionHandler {
public:
    /**
     * @brief Constructor
     * @param compression_level ZSTD compression level (1-22, default is 3)
     */
    explicit CompressionHandler(int compression_level = 3);
    
    /**
     * @brief Destructor
     */
    ~CompressionHandler();

    /**
     * @brief Compress a chunk of data
     * @param data Input data to compress
     * @return Compressed data as vector of bytes
     */
    std::vector<uint8_t> compressChunk(const std::vector<uint8_t>& data);

    /**
     * @brief Decompress a chunk of data
     * @param compressed_data Compressed input data
     * @return Decompressed data as vector of bytes
     */
    std::vector<uint8_t> decompressChunk(const std::vector<uint8_t>& compressed_data);

    /**
     * @brief Compress a file
     * @param input_path Path to input file
     * @param output_path Path to output compressed file
     * @return true if compression successful, false otherwise
     */
    bool compressFile(const std::string& input_path, const std::string& output_path);

    /**
     * @brief Decompress a file
     * @param input_path Path to compressed file
     * @param output_path Path to output decompressed file
     * @return true if decompression successful, false otherwise
     */
    bool decompressFile(const std::string& input_path, const std::string& output_path);

    /**
     * @brief Get the current compression level
     * @return Current compression level
     */
    int getCompressionLevel() const { return compression_level_; }

    /**
     * @brief Set a new compression level
     * @param level New compression level (1-22)
     */
    void setCompressionLevel(int level);

    /**
     * @brief Set compression level for a specific file extension
     * @param extension File extension (e.g., ".txt")
     * @param level Compression level (1-22)
     */
    void setExtensionCompressionLevel(const std::string& extension, int level);

    /**
     * @brief Get compression level for a specific file extension
     * @param extension File extension
     * @return Compression level for the extension, or default level if not set
     */
    int getExtensionCompressionLevel(const std::string& extension) const;

    /**
     * @brief Set minimum and maximum file sizes for compression
     * @param min_size Minimum file size in bytes
     * @param max_size Maximum file size in bytes
     */
    void setCompressionThresholds(std::uintmax_t min_size, std::uintmax_t max_size);

    /**
     * @brief Get current compression thresholds
     * @return Pair of min and max file sizes
     */
    std::pair<std::uintmax_t, std::uintmax_t> getCompressionThresholds() const;

    // Constants for chunk sizes and buffer management
    static constexpr size_t DEFAULT_CHUNK_SIZE = 4 * 1024 * 1024;  // 4MB chunks
    static constexpr size_t MAX_CHUNK_SIZE = 128 * 1024 * 1024;    // 128MB max chunk

private:
    int compression_level_;
    size_t chunk_size_;
    std::unordered_map<std::string, int> extension_levels_;
    std::uintmax_t min_file_size_;
    std::uintmax_t max_file_size_;
    
    // ZSTD context management
    ZSTD_CCtx* compression_context_;
    ZSTD_DCtx* decompression_context_;

    /**
     * @brief Initialize ZSTD contexts
     */
    void initializeContexts();

    /**
     * @brief Clean up ZSTD contexts
     */
    void cleanupContexts();

    /**
     * @brief Validate compression level
     * @param level Compression level to validate
     * @return true if valid, false otherwise
     */
    static bool isValidCompressionLevel(int level);
};

} // namespace hpe 