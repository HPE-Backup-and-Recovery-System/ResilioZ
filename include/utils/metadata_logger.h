#ifndef UTILS_METADATA_LOGGER_H_
#define UTILS_METADATA_LOGGER_H_

#include <string>
#include <filesystem>
#include <nlohmann/json.hpp>

namespace utils {

class MetadataLogger {
 public:
  static void GenerateMetadata(const std::filesystem::path& source_path,
                               const std::string& metadata_file_path);
};

} // namespace utils

#endif  // UTILS_METADATA_LOGGER_H_
