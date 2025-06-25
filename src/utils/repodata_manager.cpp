#include "utils/repodata_manager.h"

#include <cstdlib>
#include <filesystem>
#include <fstream>
#include <iostream>

#include "repositories/repository.h"
#include "utils/utils.h"

using json = nlohmann::json;
namespace fs = std::filesystem;

RepodataManager::RepodataManager() {
  try {
    std::string base_dir = Setup::GetAppDataPath() + "/.data";
    fs::create_directories(base_dir);
    data_file_ = base_dir + "/repodata.json";
    EnsureDataFileExists();
    Load();
  } catch (...) {
    ErrorUtil::ThrowNested("Failed to initialize local repository database");
  }
}

void RepodataManager::EnsureDataFileExists() {
  try {
    if (!fs::exists(data_file_)) {
      std::ofstream ofs(data_file_);
      if (!ofs) {
        ErrorUtil::ThrowError("Unable to create repository data file");
      }
      ofs << "[]";
      ofs.close();
    }
  } catch (...) {
    ErrorUtil::ThrowNested("Could not ensure presence of repository data file");
  }
}

bool RepodataManager::Load() {
  try {
    std::ifstream file(data_file_);
    if (!file.is_open()) {
      ErrorUtil::ThrowError("Could not read repository data file");
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
  } catch (...) {
    ErrorUtil::ThrowNested("Failed to load existing repositories");
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
      ErrorUtil::ThrowError("Unable to write to repository data file");
    }

    file << j.dump(2);
    return true;
  } catch (...) {
    ErrorUtil::ThrowNested("Failed to save repository information");
    return false;
  }
}

void RepodataManager::AddEntry(const RepoEntry& entry) {
  entries_.push_back(entry);
  if (!Save()) {
    ErrorUtil::ThrowError(
        "Could not update repository data after adding new entry");
  }
}

bool RepodataManager::DeleteEntry(const std::string& name,
                                  const std::string& path) {
  std::string resolved_path = Repository::GetResolvedPath(path);
  auto it =
      std::remove_if(entries_.begin(), entries_.end(), [&](const RepoEntry& e) {
        return e.name == name && e.path == resolved_path;
      });

  if (it == entries_.end()) return false;
  entries_.erase(it, entries_.end());

  if (!Save()) {
    ErrorUtil::ThrowError(
        "Could not update repository data after deleting entry");
    return false;
  }
  return true;
}

std::optional<RepoEntry> RepodataManager::GetEntry(
    const std::string& name, const std::string& path) const {
  std::string resolved_path = Repository::GetResolvedPath(path);
  for (const auto& entry : entries_) {
    if (entry.name == name && entry.path == resolved_path) return entry;
  }
  return std::nullopt;
}

std::vector<RepoEntry> RepodataManager::GetAll() const { return entries_; }
