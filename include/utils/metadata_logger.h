#ifndef METADATA_LOGGER_H_
#define METADATA_LOGGER_H_

#include <string>
#include <filesystem>
#include <nlohmann/json.hpp>



class MetadataLogger {
 public:
  static void GenerateMetadata(const std::filesystem::path& source_path,
                               const std::string& metadata_file_path);
};
 // namespace utils

#endif  //METADATA_LOGGER_H_
