#include "utils/repodata_manager.h"

#include <cstdlib>
#include <filesystem>
#include <fstream>

using json = nlohmann::json;
namespace fs = std::filesystem;

RepodataManager::RepodataManager() {
  std::string base_dir = "./data";  // TODO: Fix Path Issue (data Folder is made inside Debug)...
  fs::create_directories(base_dir);
  data_file_ = base_dir + "/repodata.json";
  EnsureDataFileExists();
  Load();
}

void RepodataManager::EnsureDataFileExists() {
  if (!fs::exists(data_file_)) {
    std::ofstream ofs(data_file_);
    ofs << "[]";
  }
}

bool RepodataManager::Load() {
  std::ifstream file(data_file_);
  if (!file.is_open()) return false;

  json j;
  file >> j;
  entries_.clear();
  for (const auto& item : j) {
    entries_.push_back({item["name"], item["path"], item["type"]});
  }
  return true;
}

bool RepodataManager::Save() {
  json j = json::array();
  for (const auto& entry : entries_) {
    j.push_back(
        {{"name", entry.name}, {"path", entry.path}, {"type", entry.type}});
  }
  std::ofstream file(data_file_);
  file << j.dump(2);
  return true;
}

void RepodataManager::AddEntry(const RepoEntry& entry) {
  entries_.push_back(entry);
  Save();
}

bool RepodataManager::DeleteEntry(const std::string& name) {
  auto it = std::remove_if(entries_.begin(), entries_.end(),
                           [&](const RepoEntry& e) { return e.name == name; });
  if (it == entries_.end()) return false;
  entries_.erase(it, entries_.end());
  return Save();
}

std::vector<RepoEntry> RepodataManager::GetAll() const { return entries_; }

std::optional<RepoEntry> RepodataManager::FindByName(
    const std::string& name) const {
  for (const auto& entry : entries_) {
    if (entry.name == name) return entry;
  }
  return std::nullopt;
}
