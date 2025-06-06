#include <utils/backup_utils.h>

#include <fstream>
#include <nlohmann/json.hpp>

namespace utils {
using json = nlohmann::json;
// Reads source directory from a JSON config file
std::string BackupUtils::GetSourcePathFromConfig(
    const std::string& config_path) {
  std::ifstream file(config_path);
  if (!file.is_open()) {
    throw std::runtime_error("Failed to open config file: " + config_path);
  }

  json config;
  file >> config;
  return config["source_path"];
}
std::string BackupUtils::GetCurrentTimestamp() {
  auto now = std::chrono::system_clock::now();
  std::time_t now_c = std::chrono::system_clock::to_time_t(now);

  std::stringstream ss;
  ss << std::put_time(std::localtime(&now_c), "%Y-%m-%d_%H-%M-%S");
  return ss.str();
}

std::string BackupUtils::GetDestPath(const std::string& config_path) {
    std::ifstream file(config_path);
    if (!file.is_open()) {
      throw std::runtime_error("Failed to open config file: " + config_path);
    }
    json config;
    file >> config;
    if (config.contains("destination_path")) {
      return config["destination_path"].get<std::string>();
    } else {
      throw std::runtime_error("Config file does not contain 'destination_path'.");
    }
  }

void BackupUtils::SetDestPathIntoConfig(const std::string& config_path,
                           const std::string& dest_path) {
  std::ifstream file(config_path);
  if (!file.is_open()) {
    throw std::runtime_error("Failed to open config file: " + config_path);
  }
  json config;
  file >> config;
  config["destination_path"] = dest_path;
  std::ofstream out_file(config_path);
  if (!out_file.is_open()) {
    throw std::runtime_error("Failed to write to config file: " + config_path);
  }
  out_file << config.dump(4);
}

}  // namespace utils