#ifndef CHUNKER_HPP_
#define CHUNKER_HPP_

#include <cstdint>
#include <filesystem>
#include <functional>
#include <vector>

namespace fs = std::filesystem;

struct Chunk {
  std::vector<uint8_t> data;
  size_t size;
  std::string hash;
};

class Chunker {
 public:
  explicit Chunker(size_t average_size = 8192);

  std::vector<Chunk> SplitFile(const fs::path& file_path);
  void CombineChunks(const std::vector<Chunk>& chunks,
                     const fs::path& output_path);

  // New streaming methods
  void StreamSplitFile(const fs::path& file_path,
                       std::function<void(const Chunk&)> chunk_callback);
  void StreamCombineChunks(std::function<Chunk()> chunk_provider,
                           const fs::path& output_path, size_t original_size);

 private:
  size_t average_chunk_size_;

  // FastCDC implementation methods
  uint64_t CalculateGearHash(const std::vector<uint8_t>& data, size_t start,
                             size_t length);
  size_t FindChunkBoundaryWithFastCDC(const std::vector<uint8_t>& data,
                                      size_t start_pos);

  // Helper methods for streaming
  void ProcessChunk(const std::vector<uint8_t>& chunk_data,
                    std::function<void(const Chunk&)> chunk_callback);
};

#endif  // CHUNKER_HPP_
