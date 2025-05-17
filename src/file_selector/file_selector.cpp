#include "file_selector.h"
#include <iostream>
#include <iomanip>
#include <chrono>
#include <algorithm>
#include <cctype>
#include <sstream>
#include <thread>
#include <pwd.h>
#include <unistd.h>

namespace fs = std::filesystem;

// Box drawing characters as UTF-8 strings
const char* const BOX_HORIZONTAL = "\xe2\x94\x80";  // ─
const char* const BOX_TOP_LEFT = "\xe2\x94\x8c";    // ╭
const char* const BOX_TOP_RIGHT = "\xe2\x94\x90";   // ╮
const char* const BOX_BOTTOM_LEFT = "\xe2\x94\x94";  // ╰
const char* const BOX_BOTTOM_RIGHT = "\xe2\x94\x98"; // ╯
const char* const BOX_VERTICAL = "\xe2\x94\x82";     // │
const char* const BOX_T_RIGHT = "\xe2\x94\x9c";      // ├
const char* const BOX_T_LEFT = "\xe2\x94\xa4";       // ┤

FileSelector::FileSelector() {}

void FileSelector::scanFileSystem() {
    fileIndex.clear();
    fileTypeIndex.clear();

    // Get user's home directory
    const char* homeDir = getenv("HOME");
    if (!homeDir) {
        struct passwd* pw = getpwuid(getuid());
        homeDir = pw->pw_dir;
    }

    // Common directories to scan
    std::vector<std::string> directories = {
        std::string(homeDir),
        "/home",
        "/usr/local",
        "/opt",
        ".",
        "./backup",
        "/bin",
        "/sbin",
        "/etc",
        "/var",
        "/tmp",
        "/usr"
    };

    for (const auto& dir : directories) {
        try {
            indexDirectory(dir);
        } catch (const fs::filesystem_error& e) {
            // Ignore inaccessible directories
        }
    }
}

void FileSelector::indexDirectory(const std::string& path) {
    try {
        for (const auto& entry : fs::recursive_directory_iterator(
            path,
            fs::directory_options::skip_permission_denied
        )) {
            if (fs::is_regular_file(entry)) {
                FileInfo info;
                info.path = entry.path().string();
                info.name = entry.path().filename().string();
                info.type = getFileType(info.path);
                try { info.size = fs::file_size(entry); } catch (...) { info.size = 0; }
                try { info.lastModified = formatTimestamp(fs::last_write_time(entry)); } catch (...) { info.lastModified = "N/A"; }
                
                fileIndex.push_back(info);
                categorizeFile(info);
            }
        }
    } catch (const fs::filesystem_error&) {
        // Skip inaccessible directories
    }
}

std::string FileSelector::getFileType(const std::string& path) const {
    std::string ext = fs::path(path).extension().string();
    if (ext.empty()) return "No Extension";
    if (ext[0] == '.') ext = ext.substr(1);
    std::transform(ext.begin(), ext.end(), ext.begin(), ::tolower);
    return ext;
}

void FileSelector::categorizeFile(const FileInfo& file) {
    fileTypeIndex[file.type].push_back(file);
}

std::string FileSelector::formatSize(uintmax_t bytes) const {
    const char* units[] = {"B", "KB", "MB", "GB", "TB"};
    int unitIndex = 0;
    double size = static_cast<double>(bytes);

    while (size >= 1024 && unitIndex < 4) {
        size /= 1024;
        unitIndex++;
    }

    std::stringstream ss;
    ss << std::fixed << std::setprecision(2) << size << " " << units[unitIndex];
    return ss.str();
}

std::string FileSelector::formatTimestamp(const fs::file_time_type& time) const {
    using namespace std::chrono;
    try {
        auto systemTime = time_point_cast<system_clock::duration>(
            time - fs::file_time_type::clock::now() + system_clock::now());
        auto tt = system_clock::to_time_t(systemTime);
        std::stringstream ss;
        ss << std::put_time(std::localtime(&tt), "%Y-%m-%d %H:%M:%S");
        return ss.str();
    } catch (...) {
        return "N/A";
    }
}

bool FileSelector::matchesQuery(const FileInfo& file, const std::string& query) const {
    if (query.empty()) return true;
    
    std::string lowerQuery = query;
    std::transform(lowerQuery.begin(), lowerQuery.end(), lowerQuery.begin(), ::tolower);
    
    std::string lowerName = file.name;
    std::transform(lowerName.begin(), lowerName.end(), lowerName.begin(), ::tolower);
    
    return lowerName.find(lowerQuery) != std::string::npos;
}

std::vector<FileInfo> FileSelector::getFilteredFiles(const std::string& searchQuery) const {
    std::vector<FileInfo> filtered;
    // Only filter if query is not empty to avoid showing everything initially
    if (searchQuery.empty()) {
        // Optionally show a limited number of initial files or none
        // For now, show nothing until search query is entered
        return {}; 
    }

    for (const auto& file : fileIndex) {
        if (matchesQuery(file, searchQuery)) {
            filtered.push_back(file);
        }
    }
    // Sort alphabetically
    std::sort(filtered.begin(), filtered.end(), [](const FileInfo& a, const FileInfo& b) {
        return a.name < b.name;
    });

    return filtered;
}

void FileSelector::displayFileSelection(const std::vector<FileInfo>& files, int selected, const std::string& searchQuery) const {
    // Move cursor to the position where the file list starts
    // This is below the search box and headers
    const int startDisplayRow = 6; // Adjust based on header height
    std::cout << "\033[" << startDisplayRow << ";" << 1 << "H";
    
    // Display search box (always redraw this part to update the query)
    std::cout << BOX_TOP_LEFT;
    for (int i = 0; i < 78; ++i) std::cout << BOX_HORIZONTAL;
    std::cout << BOX_TOP_RIGHT << std::endl;

    std::cout << BOX_VERTICAL << " Search: " << searchQuery;
    // Clear the rest of the search line and position cursor for input
    std::cout << std::string(69 - searchQuery.length(), ' ') << BOX_VERTICAL << "\033[G"; // Move to start of current line
    std::cout << "\033[" << (searchQuery.length() + 10) << "C"; // Position after Search: + query
    std::cout << std::endl;

    std::cout << BOX_T_RIGHT;
    for (int i = 0; i < 78; ++i) std::cout << BOX_HORIZONTAL;
    std::cout << BOX_T_LEFT << std::endl;

    // Display column headers
    std::cout << BOX_VERTICAL << " " << std::left 
              << std::setw(40) << "Name"
              << std::setw(10) << "Type"
              << std::setw(10) << "Size"
              << std::setw(16) << "Modified" << BOX_VERTICAL << std::endl;

    std::cout << BOX_T_RIGHT;
    for (int i = 0; i < 78; ++i) std::cout << BOX_HORIZONTAL;
    std::cout << BOX_T_LEFT << std::endl;

    // Calculate visible range
    const int displayHeight = 20; // Number of files to display
    int startIdx = std::max(0, selected - displayHeight / 2);
    int endIdx = std::min(static_cast<int>(files.size()), startIdx + displayHeight);
    
    // Adjust startIdx if we are at the end of the list
    if (files.size() > displayHeight && endIdx == files.size()) {
        startIdx = files.size() - displayHeight;
    }
     startIdx = std::max(0, startIdx); // Ensure startIdx is not negative


    // Display files
    for (int i = 0; i < displayHeight; ++i) {
        int currentFileIndex = startIdx + i;
        // Clear the current line before drawing
        std::cout << "\033[2K";
        std::cout << BOX_VERTICAL << " "; // Start of the line

        if (currentFileIndex < files.size()) {
            const auto& file = files[currentFileIndex];
            
            std::string displayName = file.name;
            if (displayName.length() > 37) {
                displayName = displayName.substr(0, 34) + "...";
            }

            if (currentFileIndex == selected) {
                std::cout << "\033[1;32m"; // Green color
            }

            std::cout << std::left
                      << std::setw(40) << displayName
                      << std::setw(10) << file.type
                      << std::setw(10) << formatSize(file.size)
                      << std::setw(16) << file.lastModified;

            if (currentFileIndex == selected) {
                std::cout << "\033[0m"; // Reset color
            }
        } else {
            // Print empty lines if fewer files than displayHeight
            std::cout << std::string(40 + 10 + 10 + 16, ' ');
        }
         std::cout << BOX_VERTICAL << std::endl; // End of the line
    }

    std::cout << BOX_BOTTOM_LEFT;
    for (int i = 0; i < 78; ++i) std::cout << BOX_HORIZONTAL;
    std::cout << BOX_BOTTOM_RIGHT << std::endl;

    // Position cursor back to the search input line
    // Move up from the current position (after the bottom box)
    int linesToMoveUp = displayHeight + 3; // Box + message
    std::cout << "\033[" << linesToMoveUp << "A";

    // Position cursor after "Search: " + current search query
     std::cout << "\033[" << (searchQuery.length() + 10) << "C";
}

bool FileSelector::addFileToRepo(const std::string& filePath, const std::string& repoPath) const {
    try {
        std::string destPath = repoPath + "/data/" + fs::path(filePath).filename().string();
        fs::copy_file(filePath, destPath, fs::copy_options::overwrite_existing);
        return true;
    } catch (const fs::filesystem_error& e) {
        std::cerr << "Error copying file: " << e.what() << std::endl;
        return false;
    }
} 