#include "chunk_hasher.h"
#include <iomanip>
#include <sstream>
#include <thread>
#include <future>

namespace hpe {
namespace backup {

// Initialize thread_local hash context
thread_local std::unique_ptr<EVP_MD_CTX, void(*)(EVP_MD_CTX*)> 
    ChunkHasher::hash_ctx_(nullptr, EVP_MD_CTX_free);

void ChunkHasher::ensure_context() {
    if (!hash_ctx_) {
        hash_ctx_.reset(EVP_MD_CTX_new());
        if (!hash_ctx_) {
            throw std::runtime_error("Failed to create EVP_MD_CTX");
        }
    }
}

std::string ChunkHasher::calculate_hash(const std::vector<uint8_t>& data) {
    ensure_context();
    
    // Initialize hash context with SHA-256
    if (EVP_DigestInit_ex(hash_ctx_.get(), EVP_sha256(), nullptr) != 1) {
        throw std::runtime_error("Failed to initialize SHA-256 context");
    }

    // Update hash with data
    if (EVP_DigestUpdate(hash_ctx_.get(), data.data(), data.size()) != 1) {
        throw std::runtime_error("Failed to update hash context");
    }

    // Finalize hash
    std::vector<unsigned char> hash(EVP_MAX_MD_SIZE);
    unsigned int hash_len = 0;
    if (EVP_DigestFinal_ex(hash_ctx_.get(), hash.data(), &hash_len) != 1) {
        throw std::runtime_error("Failed to finalize hash");
    }

    hash.resize(hash_len);
    return binary_to_hex(hash);
}

std::vector<std::string> ChunkHasher::calculate_hashes(
    const std::vector<std::vector<uint8_t>>& chunks) {
    
    // Determine optimal number of threads based on hardware and chunk count
    unsigned int hardware_threads = std::thread::hardware_concurrency();
    unsigned int num_threads = std::min(
        hardware_threads ? hardware_threads : 2,
        static_cast<unsigned int>(chunks.size())
    );

    std::vector<std::string> hashes(chunks.size());
    std::vector<std::future<void>> futures;

    // Split work among threads
    for (unsigned int i = 0; i < num_threads; ++i) {
        futures.push_back(std::async(std::launch::async, [&, i]() {
            for (size_t j = i; j < chunks.size(); j += num_threads) {
                hashes[j] = calculate_hash(chunks[j]);
            }
        }));
    }

    // Wait for all threads to complete
    for (auto& future : futures) {
        future.wait();
    }

    return hashes;
}

std::vector<std::string> ChunkHasher::calculate_stream_hashes(
    std::istream& input_stream, size_t chunk_size) {
    
    std::vector<std::string> hashes;
    std::vector<uint8_t> buffer(chunk_size);

    ensure_context();

    while (input_stream) {
        // Read chunk
        input_stream.read(reinterpret_cast<char*>(buffer.data()), chunk_size);
        std::streamsize bytes_read = input_stream.gcount();
        
        if (bytes_read > 0) {
            buffer.resize(bytes_read);
            hashes.push_back(calculate_hash(buffer));
            buffer.resize(chunk_size);  // Restore buffer size for next read
        }
    }

    return hashes;
}

std::string ChunkHasher::binary_to_hex(const std::vector<unsigned char>& binary_hash) {
    std::stringstream ss;
    ss << std::hex << std::setfill('0');
    
    for (unsigned char byte : binary_hash) {
        ss << std::setw(2) << static_cast<int>(byte);
    }
    
    return ss.str();
}

} // namespace backup
} // namespace hpe 