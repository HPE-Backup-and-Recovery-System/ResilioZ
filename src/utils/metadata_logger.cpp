#include "utils/metadata_logger.h"

#include <chrono>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <nlohmann/json.hpp>
#include <stdexcept>
namespace fs = std::filesystem;
using json = nlohmann::json;
namespace utils {
void MetadataLogger::GenerateMetadata(const fs::path& source_path,
                                      const std::string& metadata_file_path) {
  json metadata;

  try {
    for (const auto& entry : fs::recursive_directory_iterator(source_path)) {
      if (fs::is_regular_file(entry.path())) {
        auto ftime = fs::last_write_time(entry.path());
        // C++17 compatible time conversion
        auto sctp =
            std::chrono::time_point_cast<std::chrono::system_clock::duration>(
                ftime - fs::file_time_type::clock::now() +
                std::chrono::system_clock::now());
        fs::path relative_path = fs::relative(entry.path(), source_path);

        metadata["files"].push_back(
            {{"path", relative_path.string()},
             {"size", fs::file_size(entry.path())},
             {"mtime", std::chrono::duration_cast<std::chrono::seconds>(
                           sctp.time_since_epoch())
                           .count()}});
      }
    }

    // Write metadata to file
    std::ofstream file(metadata_file_path);
    if (!file) {
      throw std::runtime_error("Cannot open metadata file for writing: " +
                               metadata_file_path);
    }
    file << metadata.dump(2);

  } catch (const fs::filesystem_error& e) {
    throw std::runtime_error("Filesystem error in GenerateMetadata: " +
                             std::string(e.what()));
  } catch (const std::exception& e) {
    throw std::runtime_error("Error in GenerateMetadata: " +
                             std::string(e.what()));
  }
}
} // namespace utils
