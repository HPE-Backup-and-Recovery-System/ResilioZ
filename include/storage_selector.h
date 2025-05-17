#ifndef STORAGE_SELECTOR_H
#define STORAGE_SELECTOR_H

#include <string>
#include <vector>
#include <map>
#include <filesystem>
#include "logger.h"
#include "storageindex.h"
#include "repo_manager.h"

namespace fs = std::filesystem;

class StorageSelector {
public:
    StorageSelector();

    // Core functionality
    void scanAndIndexStorages();
    std::vector<std::string> getStorages();
    void displayStorages();
    std::string selectStorage(int index);

    // Refresh storage list (re-scan)
    void refreshStorages();

private:
    StorageIndex index;
    
    // Helper methods
    bool isValidStorageLocation(const std::string& path);
    uint64_t getAvailableSpace(const std::string& path);
    std::string getFormattedSize(uint64_t bytes);
};

#endif // STORAGE_SELECTOR_H
