#include "compression_handler.h"
#include <stdexcept>
#include <fstream>
#include <filesystem>
#include <system_error>
#include <algorithm>

namespace hpe {

CompressionHandler::CompressionHandler(int compression_level)
    : compression_level_(compression_level)
    , chunk_size_(DEFAULT_CHUNK_SIZE)
    , compression_context_(nullptr)
    , decompression_context_(nullptr)
    , min_file_size_(4096)  // 4KB default
    , max_file_size_(1ULL << 40)  // 1TB default
{
    if (!isValidCompressionLevel(compression_level)) {
        throw std::invalid_argument("Invalid compression level. Must be between 1 and 22.");
    }
    
    initializeContexts();
}

CompressionHandler::~CompressionHandler() {
    cleanupContexts();
}

void CompressionHandler::initializeContexts() {
    compression_context_ = ZSTD_createCCtx();
    if (compression_context_ == nullptr) {
        throw std::runtime_error("Failed to create ZSTD compression context");
    }

    decompression_context_ = ZSTD_createDCtx();
    if (decompression_context_ == nullptr) {
        ZSTD_freeCCtx(compression_context_);
        throw std::runtime_error("Failed to create ZSTD decompression context");
    }

    // Set compression level
    size_t result = ZSTD_CCtx_setParameter(compression_context_, ZSTD_c_compressionLevel, compression_level_);
    if (ZSTD_isError(result)) {
        cleanupContexts();
        throw std::runtime_error(std::string("Failed to set compression level: ") + ZSTD_getErrorName(result));
    }
}

void CompressionHandler::cleanupContexts() {
    if (compression_context_) {
        ZSTD_freeCCtx(compression_context_);
        compression_context_ = nullptr;
    }
    if (decompression_context_) {
        ZSTD_freeDCtx(decompression_context_);
        decompression_context_ = nullptr;
    }
}

std::vector<uint8_t> CompressionHandler::compressChunk(const std::vector<uint8_t>& data) {
    if (data.empty()) {
        return std::vector<uint8_t>();
    }

    // Get maximum compressed size
    size_t max_dst_size = ZSTD_compressBound(data.size());
    std::vector<uint8_t> compressed_data(max_dst_size);

    // Perform compression
    size_t compressed_size = ZSTD_compressCCtx(
        compression_context_,
        compressed_data.data(), compressed_data.size(),
        data.data(), data.size(),
        compression_level_
    );

    if (ZSTD_isError(compressed_size)) {
        throw std::runtime_error(std::string("Compression failed: ") + ZSTD_getErrorName(compressed_size));
    }

    // Resize to actual compressed size
    compressed_data.resize(compressed_size);
    return compressed_data;
}

std::vector<uint8_t> CompressionHandler::decompressChunk(const std::vector<uint8_t>& compressed_data) {
    if (compressed_data.empty()) {
        return std::vector<uint8_t>();
    }

    // Get original size
    unsigned long long original_size = ZSTD_getFrameContentSize(compressed_data.data(), compressed_data.size());
    if (original_size == ZSTD_CONTENTSIZE_ERROR || original_size == ZSTD_CONTENTSIZE_UNKNOWN) {
        throw std::runtime_error("Failed to determine original data size");
    }

    std::vector<uint8_t> decompressed_data(original_size);

    // Perform decompression
    size_t decompressed_size = ZSTD_decompressDCtx(
        decompression_context_,
        decompressed_data.data(), decompressed_data.size(),
        compressed_data.data(), compressed_data.size()
    );

    if (ZSTD_isError(decompressed_size)) {
        throw std::runtime_error(std::string("Decompression failed: ") + ZSTD_getErrorName(decompressed_size));
    }

    return decompressed_data;
}

bool CompressionHandler::compressFile(const std::string& input_path, const std::string& output_path) {
    try {
        // Get file size
        std::uintmax_t file_size = std::filesystem::file_size(input_path);
        
        // Check size thresholds
        if (file_size < min_file_size_ || file_size > max_file_size_) {
            // Just copy the file if it's outside our compression thresholds
            std::filesystem::copy_file(input_path, output_path, 
                                     std::filesystem::copy_options::overwrite_existing);
            return true;
        }
        
        // Get file extension and its compression level
        std::string extension = std::filesystem::path(input_path).extension().string();
        int level = getExtensionCompressionLevel(extension);
        
        // Temporarily set the compression level for this file
        int old_level = compression_level_;
        setCompressionLevel(level);
        
        std::ifstream input_file(input_path, std::ios::binary);
        if (!input_file) {
            throw std::runtime_error("Cannot open input file: " + input_path);
        }

        std::ofstream output_file(output_path, std::ios::binary);
        if (!output_file) {
            throw std::runtime_error("Cannot create output file: " + output_path);
        }

        std::vector<uint8_t> buffer(chunk_size_);
        while (input_file) {
            input_file.read(reinterpret_cast<char*>(buffer.data()), buffer.size());
            std::streamsize bytes_read = input_file.gcount();
            
            if (bytes_read > 0) {
                buffer.resize(bytes_read);
                auto compressed_chunk = compressChunk(buffer);
                output_file.write(reinterpret_cast<const char*>(compressed_chunk.data()), 
                                compressed_chunk.size());
            }
        }
        
        // Restore the original compression level
        setCompressionLevel(old_level);
        return true;
    } catch (const std::exception& e) {
        // Log error and return false
        return false;
    }
}

bool CompressionHandler::decompressFile(const std::string& input_path, const std::string& output_path) {
    try {
        std::ifstream input_file(input_path, std::ios::binary);
        if (!input_file) {
            throw std::runtime_error("Cannot open input file: " + input_path);
        }

        std::ofstream output_file(output_path, std::ios::binary);
        if (!output_file) {
            throw std::runtime_error("Cannot create output file: " + output_path);
        }

        // Get file size
        input_file.seekg(0, std::ios::end);
        std::streamsize file_size = input_file.tellg();
        input_file.seekg(0, std::ios::beg);

        // Read entire compressed file
        std::vector<uint8_t> compressed_data(file_size);
        input_file.read(reinterpret_cast<char*>(compressed_data.data()), file_size);

        auto decompressed_data = decompressChunk(compressed_data);
        output_file.write(reinterpret_cast<const char*>(decompressed_data.data()),
                         decompressed_data.size());

        return true;
    } catch (const std::exception& e) {
        // Log error and return false
        return false;
    }
}

void CompressionHandler::setCompressionLevel(int level) {
    if (!isValidCompressionLevel(level)) {
        throw std::invalid_argument("Invalid compression level. Must be between 1 and 22.");
    }
    
    compression_level_ = level;
    
    // Update compression context with new level
    size_t result = ZSTD_CCtx_setParameter(compression_context_, ZSTD_c_compressionLevel, compression_level_);
    if (ZSTD_isError(result)) {
        throw std::runtime_error(std::string("Failed to set compression level: ") + ZSTD_getErrorName(result));
    }
}

bool CompressionHandler::isValidCompressionLevel(int level) {
    return level >= 1 && level <= ZSTD_maxCLevel();
}

void CompressionHandler::setExtensionCompressionLevel(const std::string& extension, int level) {
    if (!isValidCompressionLevel(level)) {
        throw std::invalid_argument("Invalid compression level. Must be between 1 and 22.");
    }

    // Convert extension to lowercase for case-insensitive comparison
    std::string ext = extension;
    std::transform(ext.begin(), ext.end(), ext.begin(), ::tolower);
    
    // Make sure extension starts with a dot
    if (!ext.empty() && ext[0] != '.') {
        ext = "." + ext;
    }
    
    extension_levels_[ext] = level;
}

int CompressionHandler::getExtensionCompressionLevel(const std::string& extension) const {
    // Convert extension to lowercase for case-insensitive comparison
    std::string ext = extension;
    std::transform(ext.begin(), ext.end(), ext.begin(), ::tolower);
    
    // Make sure extension starts with a dot
    if (!ext.empty() && ext[0] != '.') {
        ext = "." + ext;
    }
    
    auto it = extension_levels_.find(ext);
    return it != extension_levels_.end() ? it->second : compression_level_;
}

void CompressionHandler::setCompressionThresholds(std::uintmax_t min_size, std::uintmax_t max_size) {
    if (min_size >= max_size) {
        throw std::invalid_argument("Minimum size must be less than maximum size");
    }
    
    min_file_size_ = min_size;
    max_file_size_ = max_size;
}

std::pair<std::uintmax_t, std::uintmax_t> CompressionHandler::getCompressionThresholds() const {
    return {min_file_size_, max_file_size_};
}

} // namespace hpe 