#ifndef PROJECT_ROOT_DIR
#define PROJECT_ROOT_DIR "."
#endif

#include "utils/repodata_manager.h"

#include <cstdlib>
#include <filesystem>
#include <fstream>
#include <iostream>

using json = nlohmann::json;
namespace fs = std::filesystem;

RepodataManager::RepodataManager() {
  try {
    std::string base_dir = std::string(PROJECT_ROOT_DIR) + "/data";
    fs::create_directories(base_dir);
    data_file_ = base_dir + "/repodata.json";
    EnsureDataFileExists();
    Load();
  } catch (const fs::filesystem_error& e) {
    std::cerr << "ERROR in FileSystem: " << e.what() << std::endl;
  } catch (const std::exception& e) {
    std::cerr << "ERROR: " << e.what() << std::endl;
  }
}

void RepodataManager::EnsureDataFileExists() {
  try {
    if (!fs::exists(data_file_)) {
      std::ofstream ofs(data_file_);
      if (!ofs) {
        throw std::runtime_error("Failed to Create: repodata.json");
      }
      ofs << "[]";
      ofs.close();
    }
  } catch (const std::exception& e) {
    std::cerr << "ERROR in EnsureDataFileExists: " << e.what() << std::endl;
  }
}

bool RepodataManager::Load() {
  try {
    std::ifstream file(data_file_);
    if (!file.is_open()) {
      std::cerr << "Failed to Open repodata.json for Reading." << std::endl;
      return false;
    }

    json j;
    file >> j;
    entries_.clear();
    for (const auto& item : j) {
      entries_.push_back({item.at("name"), item.at("path"), item.at("type"),
                          item.value("password_hash", ""),
                          item.at("created_at")});
    }
    return true;
  } catch (const json::exception& e) {
    std::cerr << "ERROR in JSON Parcing: " << e.what() << std::endl;
    return false;
  } catch (const std::exception& e) {
    std::cerr << "ERROR in Load(): " << e.what() << std::endl;
    return false;
  }
}

bool RepodataManager::Save() {
  try {
    json j = json::array();
    for (const auto& entry : entries_) {
      j.push_back({
          {"name", entry.name},
          {"path", entry.path},
          {"type", entry.type},
          {"password_hash", entry.password_hash},
          {"created_at", entry.created_at},
      });
    }

    std::ofstream file(data_file_);
    if (!file.is_open()) {
      std::cerr << "Failed to Open repodata.json for Writing." << std::endl;
      return false;
    }

    file << j.dump(2);
    return true;
  } catch (const std::exception& e) {
    std::cerr << "ERROR Saving repodata.json: " << e.what() << std::endl;
    return false;
  }
}

void RepodataManager::AddEntry(const RepoEntry& entry) {
  entries_.push_back(entry);
  if (!Save()) {
    std::cerr << "ERROR: Failed to Save after Add Entry of Repo Data."
              << std::endl;
  }
}

bool RepodataManager::DeleteEntry(const std::string& name,
                                  const std::string& path) {
  auto it = std::remove_if(
      entries_.begin(), entries_.end(),
      [&](const RepoEntry& e) { return e.name == name && e.path == path; });

  if (it == entries_.end()) return false;

  entries_.erase(it, entries_.end());
  if (!Save()) {
    std::cerr << "ERROR: Failed to Save after Delete Entry of Repo Data."
              << std::endl;
    return false;
  }
  return true;
}

std::vector<RepoEntry> RepodataManager::GetAll() const { return entries_; }

std::optional<RepoEntry> RepodataManager::Find(const std::string& name,
                                               const std::string& path) const {
  for (const auto& entry : entries_) {
    if (entry.name == name && entry.path == path) return entry;
  }
  return std::nullopt;
}
