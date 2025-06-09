#include "backup_restore/chunker.hpp"

#include <openssl/sha.h>

#include <algorithm>
#include <fstream>
#include <iomanip>
#include <random>
#include <sstream>

#include "utils/error_util.h"

namespace fs = std::filesystem;

// Gear hash table - precomputed random values for FastCDC
static const uint64_t GEAR_TABLE[256] = {
    0xcab06edf, 0xb2718138, 0x3c224673, 0x3b9cf4f3, 0x99309a2f, 0x4cae6426,
    0x5cd1268b, 0xfa8d5e6e, 0x3dce9096, 0x03f6d1ba, 0x10cbd5c6, 0x7a32df70,
    0x5caaf980, 0x1ee50161, 0xdb3e2adf, 0xdaa1b79b, 0x8a876bdb, 0x55214dcf,
    0x033ce45c, 0x93da2d58, 0x2c897e9b, 0x7ca38bce, 0x6ba9c6df, 0x644f3827,
    0x17919e09, 0x98991c4f, 0xb022e20c, 0xaeed89e5, 0xac46f0a2, 0x77e8ab7c,
    0x80cdb866, 0x1cf8a455, 0x342e8a7c, 0x82307545, 0x685c10bf, 0xf4b4db0d,
    0xd583f695, 0xef3be7f8, 0x6f443b74, 0xfb536307, 0xd1eebf07, 0x3fc4cbff,
    0x9c56a01f, 0x0c876401, 0x7582b5a4, 0xb67e02d9, 0xf31f1d4a, 0x308e0bfc,
    0xc2fbe865, 0x189ff266, 0xe9301f82, 0x0c99f8f2, 0xb536b229, 0xf176078b,
    0x7e638b7f, 0xb1b17b3b, 0xdc699078, 0xee113abe, 0xe05387c9, 0x834b5fb3,
    0x6577e854, 0x46310ed6, 0xe9095a8f, 0x0666ba24, 0x6f3e64d9, 0x60a137c6,
    0x00a3fe71, 0x252827d0, 0xc968a79d, 0x71adf1c7, 0xb90b26df, 0xc0b76174,
    0x53a4a968, 0x1d8cde87, 0xee076527, 0x78ada3ed, 0x2222a4cf, 0x0f20e8b1,
    0x52661029, 0x4ee67246, 0x22f83593, 0xc06b6d72, 0xe9780131, 0x46aa9013,
    0xb0192122, 0xa88b381f, 0x3b884ca7, 0x9e1188b8, 0x28e02253, 0xa19d3fc6,
    0xea459915, 0xb5b9a788, 0x96428060, 0x753524b8, 0x61c9c992, 0x6ba735d4,
    0x66ab303e, 0xbcbdd2c2, 0xe3df7ac9, 0x2f0cf65d, 0xcdf98e52, 0xb64160e8,
    0x6b8be972, 0x45602f72, 0xcbeb420e, 0xd9a2bd46, 0xb615d4a4, 0x1cfc7f69,
    0x603689d5, 0xc3bcd0d8, 0xc4d8da81, 0xa700392a, 0x27e3a0be, 0x3e7122fa,
    0x9f4ff2d6, 0x3ab159c1, 0xa3b1cc44, 0x54d2060c, 0x9f664a53, 0xb7933a53,
    0x17e0a83d, 0xab53f0f6, 0xfb54c682, 0xc2dce1fe, 0xb728b96c, 0x27a24073,
    0x35cd89cd, 0x1626c9a9, 0x9dcf73fd, 0x2a40ad38, 0x321c7bf2, 0x859f9ad2,
    0xd12d993f, 0xcb56ee3c, 0xf95e36dc, 0x8ada584b, 0x2868e9bc, 0xe2f137ee,
    0xa7ba3cae, 0xeb331d08, 0x2a2e1fc3, 0x13ed8950, 0x707abf0e, 0xf6c84db8,
    0xbe1b3e9f, 0x8a98a6ef, 0xa829daf1, 0x8f9fd9f8, 0x1d8002fb, 0xe07544a4,
    0xd69cb989, 0x030c29c2, 0x4f0e4227, 0x2b843c5a, 0x61d649fa, 0x24a23275,
    0x29ab7954, 0x1a977796, 0xafc840bb, 0x68ea74e9, 0x51e18221, 0x7e7aacb9,
    0xd83aac74, 0x16f3ffb4, 0xa1822460, 0x796e4267, 0xce57a57f, 0xdf15a7ee,
    0xf6098f14, 0x6bb45abd, 0x51933c35, 0x792d3f18, 0x4872d2de, 0xe66a579c,
    0x5750ffa9, 0x149d5472, 0x57d2e4ac, 0x9b2030bd, 0xa6befac0, 0x7eb0fa7d,
    0x5288b8de, 0xfd749b9c, 0x5389ae25, 0x90a31d56, 0x07acafbe, 0x9ffa7e2c,
    0x19a42631, 0xbc581a52, 0xc2517ad6, 0xe437de30, 0xd75eafd7, 0x8397f5ef,
    0x894d0064, 0xeae51be9, 0xa0973cf4, 0xd09dd0df, 0x654de33c, 0x99698bf2,
    0xb2be2b5c, 0x7df281a9, 0xdc5bdac7, 0xb8bc6817, 0xc2b8ac02, 0x6755088b,
    0x42fdf274, 0xd758e0a0, 0x0fe0775a, 0x3b089ae3, 0x1302b17c, 0xbbf11915,
    0x30f3ad8f, 0x8a38175b, 0x05ddabe9, 0x6647ac44, 0x49570ac5, 0x6ad85643,
    0x6062344e, 0xf9515337, 0x3ff407ae, 0x8ff0dc25, 0x2e047222, 0x3dab32fe,
    0x70899f3f, 0x594402c4, 0x7bdb81fd, 0xb93110d4, 0xe15de0ff, 0x7265b35e,
    0x0ffbffbd, 0x234ab621, 0x1ea74ed8, 0x82caa7b4, 0x3fe7fa4f, 0xa9ab690b,
    0x82e8993e, 0xa2d35adf, 0xf87827c5, 0x00172b3e, 0xa284d80b, 0x8d536c67,
    0xd63cb52d, 0xc6db6dbb, 0x523e1ba5, 0x557c6536, 0x4168f166, 0xd7acfd41,
    0xde089e30, 0xbf167903, 0x551a3200, 0xa330b700, 0x917e3ebf, 0x5a794e62,
    0xe44d3356, 0x9fcd9417, 0x30eb9b8b, 0x6e33ef51};

Chunker::Chunker(size_t average_size) : average_chunk_size_(average_size) {
  // Initialize gear table if needed (shown abbreviated above)
}

std::vector<Chunk> Chunker::SplitFile(const fs::path& file_path) {
  std::ifstream file(file_path, std::ios::binary);
  if (!file) {
    ErrorUtil::ThrowError("Could not open file: " + file_path.string());
  }

  // Read entire file into memory
  std::vector<uint8_t> data((std::istreambuf_iterator<char>(file)),
                            std::istreambuf_iterator<char>());
  file.close();

  std::vector<Chunk> chunks;
  size_t pos = 0;

  while (pos < data.size()) {
    size_t chunk_end = FindChunkBoundaryWithFastCDC(data, pos);
    if (chunk_end == pos) {
      chunk_end = std::min(pos + average_chunk_size_, data.size());
    }

    Chunk chunk;
    chunk.data =
        std::vector<uint8_t>(data.begin() + pos, data.begin() + chunk_end);
    chunk.size = chunk.data.size();

    // Calculate SHA-256 hash of the chunk
    unsigned char hash[SHA256_DIGEST_LENGTH];
    SHA256_CTX sha256;
    SHA256_Init(&sha256);
    SHA256_Update(&sha256, chunk.data.data(), chunk.data.size());
    SHA256_Final(hash, &sha256);

    // Convert hash to hex string
    std::stringstream ss;
    for (int i = 0; i < SHA256_DIGEST_LENGTH; i++) {
      ss << std::hex << std::setw(2) << std::setfill('0')
         << static_cast<int>(hash[i]);
    }
    chunk.hash = ss.str();

    chunks.push_back(std::move(chunk));
    pos = chunk_end;
  }

  return chunks;
}

void Chunker::CombineChunks(const std::vector<Chunk>& chunks,
                            const fs::path& output_path) {
  std::ofstream file(output_path, std::ios::binary);
  if (!file) {
    ErrorUtil::ThrowError("Could not create output file: " +
                          output_path.string());
  }

  for (const auto& chunk : chunks) {
    file.write(reinterpret_cast<const char*>(chunk.data.data()),
               chunk.data.size());
  }
}

uint64_t Chunker::CalculateGearHash(const std::vector<uint8_t>& data,
                                    size_t start, size_t length) {
  uint64_t hash = 0;
  for (size_t i = 0; i < length && (start + i) < data.size(); ++i) {
    hash = (hash << 1) + GEAR_TABLE[data[start + i]];
  }
  return hash;
}

size_t Chunker::FindChunkBoundaryWithFastCDC(const std::vector<uint8_t>& data,
                                             size_t start_pos) {
  const size_t MIN_SIZE = average_chunk_size_ / 2;  // FastCDC uses smaller min
  const size_t NORMAL_SIZE = average_chunk_size_;
  const size_t MAX_SIZE = average_chunk_size_ * 8;  // FastCDC uses larger max

  // FastCDC uses different masks for different regions
  const uint64_t MASK_S = (1ULL << 13) - 1;  // Small mask for first region
  const uint64_t MASK_L = (1ULL << 11) - 1;  // Large mask for second region

  size_t pos = start_pos + MIN_SIZE;
  size_t end = std::min(start_pos + MAX_SIZE, data.size());

  if (pos >= end) {
    return end;
  }

  // Initialize gear hash for the first window
  const size_t WINDOW_SIZE = 64;  // FastCDC typically uses larger windows
  uint64_t hash = 0;

  // Calculate initial hash
  size_t window_start = pos - WINDOW_SIZE;
  if (window_start < start_pos) {
    window_start = start_pos;
  }

  for (size_t i = window_start; i < pos && i < data.size(); ++i) {
    hash = (hash << 1) + GEAR_TABLE[data[i]];
  }

  // FastCDC optimization: skip two bytes at a time in the first region
  while (pos < std::min(start_pos + NORMAL_SIZE, end)) {
    // Update hash by removing old byte and adding new bytes
    if (pos >= WINDOW_SIZE) {
      // Remove the byte that's now outside the window
      uint64_t old_contribution = GEAR_TABLE[data[pos - WINDOW_SIZE]]
                                  << (WINDOW_SIZE - 1);
      hash -= old_contribution;
    }

    // Add new byte(s) - FastCDC processes 2 bytes at a time when possible
    if (pos < data.size()) {
      hash = (hash << 1) + GEAR_TABLE[data[pos]];
    }
    if (pos + 1 < data.size() && pos + 1 < end) {
      hash = (hash << 1) + GEAR_TABLE[data[pos + 1]];
      pos += 2;  // FastCDC optimization: skip 2 bytes
    } else {
      pos += 1;
    }

    // Check boundary condition with small mask
    if ((hash & MASK_S) == 0) {
      return pos;
    }
  }

  // Second region: use larger mask, process byte by byte
  while (pos < end) {
    // Update hash
    if (pos >= WINDOW_SIZE) {
      uint64_t old_contribution = GEAR_TABLE[data[pos - WINDOW_SIZE]]
                                  << (WINDOW_SIZE - 1);
      hash -= old_contribution;
    }

    if (pos < data.size()) {
      hash = (hash << 1) + GEAR_TABLE[data[pos]];
    }
    pos++;

    // Check boundary condition with large mask
    if ((hash & MASK_L) == 0) {
      return pos;
    }
  }

  return end;
}

void Chunker::StreamSplitFile(
    const fs::path& file_path,
    std::function<void(const Chunk&)> chunk_callback) {
  std::ifstream file(file_path, std::ios::binary);
  if (!file) {
    ErrorUtil::ThrowError("Could not open file: " + file_path.string());
  }

  // Get file size
  file.seekg(0, std::ios::end);
  size_t file_size = file.tellg();
  file.seekg(0, std::ios::beg);

  // If file is smaller than minimum chunk size, process it as a single chunk
  if (file_size <= average_chunk_size_ / 2) {
    std::vector<uint8_t> data(file_size);
    file.read(reinterpret_cast<char*>(data.data()), file_size);
    ProcessChunk(data, chunk_callback);
    return;
  }

  const size_t buffer_size =
      average_chunk_size_ * 1024;  // Buffer size for reading
  std::vector<uint8_t> buffer(buffer_size);
  std::vector<uint8_t> current_chunk;
  current_chunk.reserve(average_chunk_size_);
  size_t total_bytes_processed = 0;

  while (file) {
    file.read(reinterpret_cast<char*>(buffer.data()), buffer_size);
    size_t bytes_read = file.gcount();

    if (bytes_read == 0) break;

    size_t pos = 0;
    while (pos < bytes_read) {
      size_t chunk_end = FindChunkBoundaryWithFastCDC(buffer, pos);
      if (chunk_end == pos) {
        chunk_end = std::min(pos + average_chunk_size_, bytes_read);
      }

      // Add data to current chunk
      current_chunk.insert(current_chunk.end(), buffer.begin() + pos,
                           buffer.begin() + chunk_end);

      total_bytes_processed += (chunk_end - pos);

      // Process the chunk if:
      // 1. It's large enough (>= MIN_SIZE)
      // 2. It's the last chunk in the file
      bool is_last_chunk = (total_bytes_processed == file_size);

      if (current_chunk.size() >= average_chunk_size_ / 2 || is_last_chunk) {
        ProcessChunk(current_chunk, chunk_callback);
        current_chunk.clear();
      }

      pos = chunk_end;
    }
  }

  // Process any remaining data
  if (!current_chunk.empty()) {
    ProcessChunk(current_chunk, chunk_callback);
  }
}

void Chunker::StreamCombineChunks(std::function<Chunk()> chunk_provider,
                                  const fs::path& output_path,
                                  size_t original_size) {
  std::ofstream file(output_path, std::ios::binary);
  if (!file) {
    ErrorUtil::ThrowError("Could not create output file: " +
                          output_path.string());
  }

  size_t total_bytes_written = 0;

  while (true) {
    Chunk chunk = chunk_provider();
    if (chunk.data.empty()) break;  // End of chunks

    // Calculate how many bytes we can write
    size_t bytes_to_write =
        std::min(chunk.size, original_size - total_bytes_written);

    if (bytes_to_write > 0) {
      file.write(reinterpret_cast<const char*>(chunk.data.data()),
                 bytes_to_write);
      total_bytes_written += bytes_to_write;
    }

    // if (total_bytes_written >= original_size) {
    //     break;
    // }
  }

  // Ensure the file is exactly the original size
  file.close();
  // fs::resize_file(output_path, original_size);
}

void Chunker::ProcessChunk(const std::vector<uint8_t>& chunk_data,
                           std::function<void(const Chunk&)> chunk_callback) {
  Chunk chunk;
  chunk.data = chunk_data;
  chunk.size = chunk_data.size();

  // Calculate SHA-256 hash of the chunk
  unsigned char hash[SHA256_DIGEST_LENGTH];
  SHA256_CTX sha256;
  SHA256_Init(&sha256);
  SHA256_Update(&sha256, chunk.data.data(), chunk.data.size());
  SHA256_Final(hash, &sha256);

  // Convert hash to hex string
  std::stringstream ss;
  for (int i = 0; i < SHA256_DIGEST_LENGTH; i++) {
    ss << std::hex << std::setw(2) << std::setfill('0')
       << static_cast<int>(hash[i]);
  }
  chunk.hash = ss.str();

  chunk_callback(chunk);
}
