#pragma once

#include <string>
#include <vector>
#include <memory>
#include <openssl/evp.h>
#include <openssl/sha.h>

namespace hpe {
namespace backup {

/**
 * @brief Efficient chunk hasher using OpenSSL EVP interface
 * 
 * This class provides thread-safe, efficient chunk hashing using OpenSSL's EVP
 * interface. It supports parallel hashing of multiple chunks and maintains a
 * thread-local context pool for performance.
 */
class ChunkHasher {
public:
    ChunkHasher() = default;
    ~ChunkHasher() = default;

    /**
     * @brief Calculate SHA-256 hash of a chunk
     * @param data Chunk data
     * @return Hex string of SHA-256 hash
     */
    std::string calculate_hash(const std::vector<uint8_t>& data);

    /**
     * @brief Calculate SHA-256 hashes for multiple chunks in parallel
     * @param chunks Vector of chunk data
     * @return Vector of hex string hashes
     */
    std::vector<std::string> calculate_hashes(const std::vector<std::vector<uint8_t>>& chunks);

    /**
     * @brief Stream-based hash calculation for large files
     * @param input_stream Input stream to hash
     * @param chunk_size Size of each chunk
     * @return Vector of hex string hashes for each chunk
     */
    std::vector<std::string> calculate_stream_hashes(std::istream& input_stream, size_t chunk_size);

private:
    // Thread-local hash context
    static thread_local std::unique_ptr<EVP_MD_CTX, void(*)(EVP_MD_CTX*)> hash_ctx_;
    
    /**
     * @brief Initialize thread-local hash context if needed
     */
    void ensure_context();

    /**
     * @brief Convert binary hash to hex string
     * @param binary_hash Binary hash data
     * @return Hex string representation
     */
    static std::string binary_to_hex(const std::vector<unsigned char>& binary_hash);
};

} // namespace backup
} // namespace hpe 