#pragma once
#include <vector>
#include <filesystem>
#include <cstdint>
#include <functional>


struct Chunk {
    std::vector<uint8_t> data;
    size_t size;
    std::string hash;
};

class Chunker {
public:
    explicit Chunker(size_t average_size = 8192);
    
    std::vector<Chunk> split_file(const std::filesystem::path& file_path);
    void combine_chunks(const std::vector<Chunk>& chunks,
                       const std::filesystem::path& output_path);
    
    // New streaming methods
    void stream_split_file(const std::filesystem::path& file_path,
                          std::function<void(const Chunk&)> chunk_callback);
    void stream_combine_chunks(std::function<Chunk()> chunk_provider,
                             const std::filesystem::path& output_path,
                             size_t original_size);

private:
    size_t average_chunk_size_;
    
    // FastCDC implementation methods
    uint64_t calculate_gear_hash(const std::vector<uint8_t>& data,
                                size_t start,
                                size_t length);
    
    size_t find_chunk_boundary_fastcdc(const std::vector<uint8_t>& data,
                                      size_t start_pos);
    
    // Helper methods for streaming
    void process_chunk(const std::vector<uint8_t>& chunk_data,
                      std::function<void(const Chunk&)> chunk_callback);
};
