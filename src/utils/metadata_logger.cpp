#include "utils/metadata_logger.h"

#include <fstream>
#include <iostream>
#include <filesystem>
#include <nlohmann/json.hpp>

namespace fs = std::filesystem;
using json = nlohmann::json;



void MetadataLogger::GenerateMetadata(const fs::path& source_path,
                                      const std::string& metadata_file_path) {
  json metadata;

  for (const auto& entry : fs::recursive_directory_iterator(source_path)) {
    if (fs::is_regular_file(entry.path())) {
      auto ftime = fs::last_write_time(entry.path());
      auto sctp = std::chrono::time_point_cast<std::chrono::system_clock::duration>(
          ftime - fs::file_time_type::clock::now() + std::chrono::system_clock::now()
      );
      fs::path relative_path = fs::relative(entry.path(), source_path);
      metadata["files"].push_back({
          {"path", relative_path.string()},
          {"size", fs::file_size(entry.path())},
          {"mtime", std::chrono::duration_cast<std::chrono::seconds>(sctp.time_since_epoch()).count()}
      });
    }
  }

  std::ofstream ofs(metadata_file_path);
  if (!ofs.is_open()) {
    throw std::runtime_error("Failed to write metadata to " + metadata_file_path);
  }

  ofs << metadata.dump(4);
  ofs.close();
}

// namespace utils
