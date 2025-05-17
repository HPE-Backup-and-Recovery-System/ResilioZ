#include "storage_selector.h"
#include <iostream>
#include <filesystem>
#include <fstream>
#include <vector>
#include <string>
#include <algorithm>
#include <array>
#include <cmath>
#include <iomanip>
#include "logger.h"
#include <sys/statvfs.h>
#include <unistd.h>
#include <pwd.h>

namespace fs = std::filesystem;

// Box drawing characters as UTF-8 strings
const char* const BOX_HORIZONTAL = "─";
const char* const BOX_TOP_LEFT = "╭";
const char* const BOX_TOP_RIGHT = "╮";
const char* const BOX_BOTTOM_LEFT = "╰";
const char* const BOX_BOTTOM_RIGHT = "╯";
const char* const BOX_VERTICAL = "│";

StorageSelector::StorageSelector() : index() {}

void StorageSelector::scanAndIndexStorages() {
    std::cout << "\nScanning for storage devices..." << std::endl;
    
    // Get current user's home directory
    const char* homeDir = getenv("HOME");
    if (!homeDir) {
        struct passwd* pw = getpwuid(getuid());
        homeDir = pw->pw_dir;
    }

    // Common mount points to check
    std::vector<std::string> mountPoints = {
        "/mnt",
        "/media",
        "/home",
        "/backup",
        "/data",
        std::string(homeDir),
        ".",  // Current directory
        "./backup"  // Local backup directory
    };

    // Create local backup directory if it doesn't exist
    try {
        if (!fs::exists("backup")) {
            fs::create_directory("backup");
            fs::permissions("backup", 
                fs::perms::owner_read | fs::perms::owner_write | fs::perms::owner_exec |
                fs::perms::group_read | fs::perms::group_write | fs::perms::group_exec,
                fs::perm_options::add);
        }
    } catch (const fs::filesystem_error& e) {
        std::cerr << "Failed to create local backup directory: " << e.what() << std::endl;
    }

    for (const auto& basePath : mountPoints) {
        if (!fs::exists(basePath)) continue;
        
        try {
            if (isValidStorageLocation(basePath)) {
                std::string path = fs::canonical(basePath).string();
                std::cout << "Storage detected: " << path << std::endl;
                index.insert("storage", path);
            }

            // Check subdirectories for additional storage points
            for (const auto& entry : fs::directory_iterator(basePath)) {
                if (fs::is_directory(entry.path())) {
                    std::string path = fs::canonical(entry.path()).string();
                    if (isValidStorageLocation(path)) {
                        std::cout << "Storage detected: " << path << std::endl;
                        index.insert("storage", path);
                    }
                }
            }
        } catch (const fs::filesystem_error& e) {
            std::cerr << "Error scanning " << basePath << ": " << e.what() << std::endl;
        }
    }
}

bool StorageSelector::isValidStorageLocation(const std::string& path) {
    try {
        // Check if directory exists and is writable
        if (!fs::exists(path) || !fs::is_directory(path)) {
            return false;
        }

        // Check permissions
        fs::perms p = fs::status(path).permissions();
        if ((p & fs::perms::owner_write) == fs::perms::none) {
            return false;
        }

        // Check available space (minimum 100MB)
        if (getAvailableSpace(path) < 100 * 1024 * 1024) {
            return false;
        }

        // Test write permission by creating a temporary file
        std::string testFile = path + "/.write_test";
        {
            std::ofstream test(testFile.c_str());
            if (!test.is_open()) {
                return false;
            }
            test.close();
            fs::remove(testFile);
        }

        return true;
    } catch (const fs::filesystem_error&) {
        return false;
    }
}

uint64_t StorageSelector::getAvailableSpace(const std::string& path) {
    struct statvfs stat;
    if (statvfs(path.c_str(), &stat) != 0) {
        return 0;
    }
    return stat.f_bsize * stat.f_bavail;
}

std::string StorageSelector::getFormattedSize(uint64_t bytes) {
    const char* units[] = {"B", "KB", "MB", "GB", "TB"};
    int unitIndex = 0;
    double size = bytes;

    while (size >= 1024 && unitIndex < 4) {
        size /= 1024;
        unitIndex++;
    }

    char buffer[32];
    snprintf(buffer, sizeof(buffer), "%.2f %s", size, units[unitIndex]);
    return std::string(buffer);
}

std::vector<std::string> StorageSelector::getStorages() {
    return index.search("storage");
}

void StorageSelector::displayStorages() {
    std::vector<std::string> storages = getStorages();
    
    if (storages.empty()) {
        std::cout << "No storage devices detected." << std::endl;
        return;
    }

    const size_t MAX_PATH_LENGTH = 35;
    const size_t MAX_SIZE_LENGTH = 10;
    const size_t TOTAL_WIDTH = 55;

    std::cout << "\nAvailable Storage Locations:" << std::endl;
    std::cout << BOX_TOP_LEFT;
    for (size_t i = 0; i < TOTAL_WIDTH; ++i) std::cout << BOX_HORIZONTAL;
    std::cout << BOX_TOP_RIGHT << std::endl;
    
    for (size_t i = 0; i < storages.size(); i++) {
        std::string displayPath = storages[i];
        if (displayPath.length() > MAX_PATH_LENGTH) {
            displayPath = "..." + displayPath.substr(displayPath.length() - MAX_PATH_LENGTH + 4);
        }
        
        std::string freeSpace = getFormattedSize(getAvailableSpace(storages[i]));
        if (freeSpace.length() > MAX_SIZE_LENGTH) {
            freeSpace = freeSpace.substr(0, MAX_SIZE_LENGTH - 3) + "...";
        }
        
        // Calculate padding with bounds checking
        size_t pathPadding = std::max(1UL, MAX_PATH_LENGTH - displayPath.length());
        size_t sizePadding = std::max(1UL, MAX_SIZE_LENGTH - freeSpace.length());
        
        std::cout << BOX_VERTICAL << " " << (i + 1) << ". " << displayPath
                  << std::string(pathPadding, ' ')
                  << "Free: " << freeSpace << std::string(sizePadding, ' ')
                  << BOX_VERTICAL << std::endl;
        }
    
    std::cout << BOX_BOTTOM_LEFT;
    for (size_t i = 0; i < TOTAL_WIDTH; ++i) std::cout << BOX_HORIZONTAL;
    std::cout << BOX_BOTTOM_RIGHT << std::endl;
}

std::string StorageSelector::selectStorage(int index) {
    auto storages = getStorages();
    if (index >= 0 && index < storages.size()) {
        return storages[index];
    }
    return "";
}
