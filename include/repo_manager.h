#ifndef REPO_MANAGER_H
#define REPO_MANAGER_H

#include <string>
#include <vector>
#include <filesystem>
#include <chrono>
#include "file_selector.h"
#include "storage_selector.h"
#include "storageindex.h"
#include "logger.h"

struct RepoInfo {
    std::string name;
    std::string path;
    uint64_t size;
    std::time_t lastBackup;
};

class RepoManager {
public:
    // Constructor that accepts a storage path
    explicit RepoManager(const std::string& storagePath);

    // Core repository operations
    bool createRepo(const std::string& repoName);
    std::vector<RepoInfo> listRepos();
    void displayRepos();
    bool selectRepo(const std::string& repoName);
    std::string getCurrentRepoPath() const;
    bool handleFileSelection();

    // Add a file to a repository
    bool addFileToRepo(const std::string& repoName, const std::string& filePath);

    // List all files in a repository
    std::vector<std::string> listFilesInRepo(const std::string& repoName);

private:
    std::string storagePath;
    std::string currentRepoPath;
    StorageIndex index;

    // Repository management helpers
    bool createRepoInfoFile(const std::string& repoPath, const std::string& repoName);
    bool isValidRepo(const std::string& repoPath);
    bool isValidRepoName(const std::string& name);
    uint64_t getRepoSize(const std::string& repoPath);
    std::time_t getLastBackupTime(const std::string& repoPath);
    
    // Formatting helpers
    std::string formatSize(uint64_t bytes);
    std::string formatTimestamp(std::time_t timestamp);
};

#endif // REPO_MANAGER_H
