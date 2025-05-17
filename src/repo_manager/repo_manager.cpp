#include "repo_manager.h"
#include "logger.h"
#include <iostream>
#include <filesystem>
#include <fstream>
#include <chrono>
#include <iomanip>
#include <sstream>
#include <algorithm>
#include "file_selector.h"
#include <termios.h>
#include <unistd.h>

namespace fs = std::filesystem;

// Box drawing characters as UTF-8 strings
const char* const BOX_HORIZONTAL = "─";
const char* const BOX_TOP_LEFT = "╭";
const char* const BOX_TOP_RIGHT = "╮";
const char* const BOX_BOTTOM_LEFT = "╰";
const char* const BOX_BOTTOM_RIGHT = "╯";
const char* const BOX_VERTICAL = "│";

RepoManager::RepoManager(const std::string& storagePath) : storagePath(storagePath) {}

bool RepoManager::createRepo(const std::string& repoName) {
    std::string repoPath = storagePath + "/" + repoName;
    
    // Validate repository name
    if (!isValidRepoName(repoName)) {
        Logger::log("Invalid repository name: " + repoName + ". Use only letters, numbers, dashes, and underscores.", LogLevel::ERROR);
        std::cout << "\nInvalid repository name. Please use only letters, numbers, dashes, and underscores." << std::endl;
        return false;
    }

    // Verify storage path exists and is writable
    if (!fs::exists(storagePath)) {
        Logger::log("Storage path does not exist: " + storagePath, LogLevel::ERROR);
        std::cout << "\nStorage location does not exist: " << storagePath << std::endl;
        return false;
    }

    try {
        // Test write permission by creating a temporary file
        std::string testFile = storagePath + "/.write_test";
        {
            std::ofstream test(testFile);
            if (!test.is_open()) {
                Logger::log("No write permission in storage path: " + storagePath, LogLevel::ERROR);
                std::cout << "\nNo permission to write in storage location. Please check permissions." << std::endl;
                return false;
            }
            test.close();
            fs::remove(testFile);
        }
    } catch (const std::exception& e) {
        Logger::log("Permission test failed: " + std::string(e.what()), LogLevel::ERROR);
        std::cout << "\nFailed to verify write permissions: " << e.what() << std::endl;
        return false;
    }

    // Check if repository already exists
    if (fs::exists(repoPath)) {
        Logger::log("Repository already exists: " + repoPath, LogLevel::WARNING);
        std::cout << "\nRepository already exists at: " << repoPath << std::endl;
        return false;
    }

    try {
        // Create main repository directory with full permissions
        fs::create_directories(repoPath);
        fs::permissions(repoPath, 
            fs::perms::owner_read | fs::perms::owner_write | fs::perms::owner_exec |
            fs::perms::group_read | fs::perms::group_write | fs::perms::group_exec,
            fs::perm_options::add);

    if (!fs::exists(repoPath)) {
            Logger::log("Failed to create repository directory: " + repoPath, LogLevel::ERROR);
            std::cout << "\nFailed to create repository directory. Please check permissions." << std::endl;
            return false;
        }

        // Create repository structure with error checking
        std::vector<std::string> subdirs = {"data", "metadata", "snapshots", "temp"};
        for (const auto& dir : subdirs) {
            std::string subPath = repoPath + "/" + dir;
            fs::create_directory(subPath);
            
            // Set permissions for subdirectories
            fs::permissions(subPath, 
                fs::perms::owner_read | fs::perms::owner_write | fs::perms::owner_exec |
                fs::perms::group_read | fs::perms::group_write | fs::perms::group_exec,
                fs::perm_options::add);

            if (!fs::exists(subPath)) {
                Logger::log("Failed to create subdirectory: " + subPath, LogLevel::ERROR);
                // Cleanup on failure
                fs::remove_all(repoPath);
                std::cout << "\nFailed to create repository structure at: " << subPath << std::endl;
                return false;
            }
        }

        // Create repository info file
        if (!createRepoInfoFile(repoPath, repoName)) {
            // Cleanup on failure
            fs::remove_all(repoPath);
            std::cout << "\nFailed to create repository information file." << std::endl;
            return false;
        }

        Logger::log("Repository created successfully: " + repoPath, LogLevel::INFO);
                currentRepoPath = repoPath;
        std::cout << "\nRepository created successfully at: " << repoPath << std::endl;
                return true;

        } catch (const fs::filesystem_error& e) {
        Logger::log("Filesystem error creating repository: " + std::string(e.what()), LogLevel::ERROR);
        std::cout << "\nError creating repository: " << e.what() << std::endl;
        // Cleanup on failure
        if (fs::exists(repoPath)) {
            try {
                fs::remove_all(repoPath);
            } catch (...) {
                // Ignore cleanup errors
            }
        }
        return false;
    } catch (const std::exception& e) {
            Logger::log("Error creating repository: " + std::string(e.what()), LogLevel::ERROR);
        std::cout << "\nUnexpected error creating repository: " << e.what() << std::endl;
        if (fs::exists(repoPath)) {
            try {
                fs::remove_all(repoPath);
            } catch (...) {
                // Ignore cleanup errors
            }
        }
        return false;
    }
}

bool RepoManager::createRepoInfoFile(const std::string& repoPath, const std::string& repoName) {
    try {
        std::string infoPath = repoPath + "/metadata/repo.info";
        std::ofstream info(infoPath);
        
        if (!info.is_open()) {
            Logger::log("Failed to create repo.info file: " + infoPath, LogLevel::ERROR);
    return false;
}

        auto now = std::chrono::system_clock::now();
        auto nowTime = std::chrono::system_clock::to_time_t(now);
        
        info << "Repository Name: " << repoName << "\n";
        info << "Created: " << std::put_time(std::localtime(&nowTime), "%Y-%m-%d %H:%M:%S") << "\n";
        info << "Version: 1.0\n";
        info << "Format: HPE Backup Format v1\n";
        info << "Storage Path: " << storagePath << "\n";
        
        info.close();
        return true;
    } catch (const std::exception& e) {
        Logger::log("Error creating repo.info file: " + std::string(e.what()), LogLevel::ERROR);
        return false;
    }
}

std::vector<RepoInfo> RepoManager::listRepos() {
    std::vector<RepoInfo> repos;
    
    try {
    for (const auto& entry : fs::directory_iterator(storagePath)) {
        if (fs::is_directory(entry.path())) {
                std::string repoPath = entry.path().string();
                std::string repoName = entry.path().filename().string();
                
                if (isValidRepo(repoPath)) {
                    RepoInfo info;
                    info.name = repoName;
                    info.path = repoPath;
                    info.size = getRepoSize(repoPath);
                    info.lastBackup = getLastBackupTime(repoPath);
                    repos.push_back(info);
        }
    }
        }
    } catch (const fs::filesystem_error& e) {
        Logger::log("Error listing repositories: " + std::string(e.what()), LogLevel::ERROR);
    }
    
    return repos;
}

void RepoManager::displayRepos() {
    auto repos = listRepos();
    
    if (repos.empty()) {
        std::cout << "No repositories found." << std::endl;
        return;
    }

    const size_t MAX_NAME_LENGTH = 25;
    const size_t MAX_SIZE_LENGTH = 10;
    const size_t MAX_DATE_LENGTH = 15;
    const size_t TOTAL_WIDTH = 70;

    std::cout << "\nAvailable Repositories:" << std::endl;
    std::cout << BOX_TOP_LEFT;
    for (size_t i = 0; i < TOTAL_WIDTH; ++i) std::cout << BOX_HORIZONTAL;
    std::cout << BOX_TOP_RIGHT << std::endl;
    
    for (size_t i = 0; i < repos.size(); i++) {
        const auto& repo = repos[i];
        
        // Truncate repository name if too long
        std::string displayName = repo.name;
        if (displayName.length() > MAX_NAME_LENGTH) {
            displayName = displayName.substr(0, MAX_NAME_LENGTH - 3) + "...";
        }
        
        std::string sizeStr = formatSize(repo.size);
        if (sizeStr.length() > MAX_SIZE_LENGTH) {
            sizeStr = sizeStr.substr(0, MAX_SIZE_LENGTH - 3) + "...";
        }
        
        std::string lastBackupStr = formatTimestamp(repo.lastBackup);
        if (lastBackupStr.length() > MAX_DATE_LENGTH) {
            lastBackupStr = lastBackupStr.substr(0, MAX_DATE_LENGTH - 3) + "...";
        }
        
        // Calculate padding with bounds checking
        size_t namePadding = std::max(1UL, MAX_NAME_LENGTH - displayName.length());
        size_t sizePadding = std::max(1UL, MAX_SIZE_LENGTH - sizeStr.length());
        size_t datePadding = std::max(1UL, MAX_DATE_LENGTH - lastBackupStr.length());
        
        std::cout << BOX_VERTICAL << " " << (i + 1) << ". " << displayName
                  << std::string(namePadding, ' ')
                  << "Size: " << sizeStr << std::string(sizePadding, ' ')
                  << "Last: " << lastBackupStr << std::string(datePadding, ' ')
                  << BOX_VERTICAL << std::endl;
    }
    
    std::cout << BOX_BOTTOM_LEFT;
    for (size_t i = 0; i < TOTAL_WIDTH; ++i) std::cout << BOX_HORIZONTAL;
    std::cout << BOX_BOTTOM_RIGHT << std::endl;
}

bool RepoManager::selectRepo(const std::string& repoName) {
    std::string repoPath = storagePath + "/" + repoName;
    
    if (isValidRepo(repoPath)) {
        currentRepoPath = repoPath;
        Logger::log("Repository selected: " + repoPath, LogLevel::INFO);
        return true;
    }
    
    Logger::log("Invalid repository selected: " + repoPath, LogLevel::WARNING);
    return false;
}

bool RepoManager::isValidRepo(const std::string& repoPath) {
    try {
        if (!fs::exists(repoPath) || !fs::is_directory(repoPath)) {
            return false;
        }

        // Check for required directories
        std::vector<std::string> requiredDirs = {"data", "metadata", "snapshots", "temp"};
        for (const auto& dir : requiredDirs) {
            if (!fs::exists(repoPath + "/" + dir) || !fs::is_directory(repoPath + "/" + dir)) {
                return false;
            }
        }

        // Check for repo.info file
        if (!fs::exists(repoPath + "/metadata/repo.info")) {
            return false;
        }

        return true;
    } catch (const fs::filesystem_error&) {
        return false;
    }
}

bool RepoManager::isValidRepoName(const std::string& name) {
    if (name.empty() || name.length() > 64) return false;
    
    // Check if name contains only alphanumeric characters, dashes, and underscores
    return std::all_of(name.begin(), name.end(), [](char c) {
        return std::isalnum(c) || c == '-' || c == '_';
    });
}

uint64_t RepoManager::getRepoSize(const std::string& repoPath) {
    uint64_t totalSize = 0;
    try {
        for (const auto& entry : fs::recursive_directory_iterator(repoPath)) {
            if (fs::is_regular_file(entry.status())) {
                totalSize += fs::file_size(entry.path());
            }
        }
    } catch (const fs::filesystem_error&) {}
    return totalSize;
}

std::time_t RepoManager::getLastBackupTime(const std::string& repoPath) {
    std::time_t lastBackup = 0;
    try {
        std::string snapshotsDir = repoPath + "/snapshots";
        if (!fs::exists(snapshotsDir)) return 0;

        for (const auto& entry : fs::directory_iterator(snapshotsDir)) {
            if (fs::is_directory(entry.status())) {
                auto ftime = fs::last_write_time(entry.path());
                // Convert to time_t using duration cast
                auto duration = ftime.time_since_epoch();
                auto seconds = std::chrono::duration_cast<std::chrono::seconds>(duration);
                auto tt = seconds.count();
                
                if (tt > lastBackup) {
                    lastBackup = tt;
                }
            }
        }
    } catch (const fs::filesystem_error&) {}
    return lastBackup;
}

std::string RepoManager::formatSize(uint64_t bytes) {
    const char* units[] = {"B", "KB", "MB", "GB", "TB"};
    int unitIndex = 0;
    double size = bytes;

    while (size >= 1024 && unitIndex < 4) {
        size /= 1024;
        unitIndex++;
    }

    std::stringstream ss;
    ss << std::fixed << std::setprecision(2) << size << " " << units[unitIndex];
    return ss.str();
}

std::string RepoManager::formatTimestamp(std::time_t timestamp) {
    if (timestamp == 0) return "Never";
    
    auto now = std::chrono::system_clock::now();
    auto nowTime = std::chrono::system_clock::to_time_t(now);
    
    // If backup was today, show time only
    auto tm = *std::localtime(&timestamp);
    auto tmNow = *std::localtime(&nowTime);
    
    std::stringstream ss;
    if (tm.tm_year == tmNow.tm_year && tm.tm_yday == tmNow.tm_yday) {
        ss << std::put_time(&tm, "%H:%M:%S");
    } else {
        ss << std::put_time(&tm, "%Y-%m-%d");
    }
    return ss.str();
}

std::string RepoManager::getCurrentRepoPath() const {
    return currentRepoPath;
}

bool RepoManager::handleFileSelection() {
    FileSelector fileSelector;
    std::cout << "Scanning file system..." << std::endl;
    fileSelector.scanFileSystem();

    std::string searchQuery;
    int selected = 0;
    bool selecting = true;
    bool needsRedraw = true; // Initial redraw is always needed
    std::vector<FileInfo> filteredFiles;

    // Set up terminal for raw input
    struct termios oldSettings, newSettings;
    tcgetattr(STDIN_FILENO, &oldSettings);
    newSettings = oldSettings;
    newSettings.c_lflag &= ~(ICANON | ECHO); // Disable canonical mode (line buffering) and echoing
    newSettings.c_cc[VMIN] = 0; // Read returns immediately if no bytes available
    newSettings.c_cc[VTIME] = 1; // Wait for 0.1 seconds for input

    tcsetattr(STDIN_FILENO, TCSANOW, &newSettings);

    while (selecting) {
        if (needsRedraw) {
            filteredFiles = fileSelector.getFilteredFiles(searchQuery);
            // Ensure selected index is valid after filtering
            if (!filteredFiles.empty()) {
                 if (selected >= filteredFiles.size()) {
                    selected = filteredFiles.size() - 1;
                }
                if (selected < 0) selected = 0;
            } else {
                selected = 0; // No files, no selection
            }


            fileSelector.displayFileSelection(filteredFiles, selected, searchQuery);
            needsRedraw = false;
        }

        char c = 0;
        int bytesRead = read(STDIN_FILENO, &c, 1);

        if (bytesRead > 0) {
            if (c == '\033') { // ESC sequence
                 // After reading ESC, check for immediate subsequent bytes
                 char sequence[2];
                 // Set a very short timeout to check for sequence bytes without blocking long
                 newSettings.c_cc[VTIME] = 1; // 0.1 second timeout
                 tcsetattr(STDIN_FILENO, TCSANOW, &newSettings);

                 int len = read(STDIN_FILENO, sequence, 2); // Attempt to read up to 2 more bytes

                 // Restore original VTIME for the main loop
                 newSettings.c_cc[VTIME] = 1; // Reset to 0.1 second timeout for general input
                 tcsetattr(STDIN_FILENO, TCSANOW, &newSettings);

                 if (len == 2 && sequence[0] == '[') { // Likely an arrow key sequence
                     if (sequence[1] == 'A') { // Up arrow
                         if (selected > 0) selected--;
                         needsRedraw = true;
                     } else if (sequence[1] == 'B') { // Down arrow
                         if (selected < filteredFiles.size() - 1) selected++;
                         needsRedraw = true;
                     } else {
                         // Unhandled escape sequence, ignore and don't redraw
                         needsRedraw = false;
                     }
                 } else {
                     // It was likely a single ESC key press
                     selecting = false; // Exit selection
                 }

            } else if (c == '\n' || c == '\r') { // Enter key
                if (!filteredFiles.empty() && selected >= 0 && selected < filteredFiles.size()) {
                    const auto& selectedFile = filteredFiles[selected];
                    // Restore terminal settings before printing messages
                    tcsetattr(STDIN_FILENO, TCSANOW, &oldSettings);
                    // Clear the selection UI before printing success/failure message
                    std::cout << "\033[2J\033[H"; // Clear screen and move to top
                    if (fileSelector.addFileToRepo(selectedFile.path, currentRepoPath)) {
                        std::cout << "File added successfully: " << selectedFile.name << std::endl;
                        sleep(1);
                        return true; // File added, exit selection
                    } else {
                         std::cout << "Failed to add file to repository." << std::endl;
                         sleep(1);
                         // Restore original settings again just in case before returning to main menu
                         tcsetattr(STDIN_FILENO, TCSANOW, &oldSettings);
                         return false; // Indicate failure or cancellation
                    }
                } else {
                     // Enter pressed with no files or invalid selection.
                     // Consume any pending input to prevent issues in the next menu.
                     tcflush(STDIN_FILENO, TCIFLUSH);
                      needsRedraw = false;
                 }
            } else if (c == 127) { // Backspace
                if (!searchQuery.empty()) {
                    searchQuery.pop_back();
                    needsRedraw = true; // Redraw to update search query display
                } else {
                    needsRedraw = false; // No change, no redraw
                }
            } else if (isprint(c)) { // Regular character
                searchQuery += c;
                needsRedraw = true; // Redraw to update search query display and file list
            } else {
                // Other non-printable characters, ignore and do not redraw
                needsRedraw = false;
            }
        }
        // No need for a usleep here, VTIME handles the timeout for read
    }

    // Restore original terminal settings if exiting loop via Escape
    tcsetattr(STDIN_FILENO, TCSANOW, &oldSettings);
    return false; // Selection canceled
}
