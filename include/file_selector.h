#ifndef FILE_SELECTOR_H
#define FILE_SELECTOR_H

#include <string>
#include <vector>
#include <filesystem>
#include <functional>
#include <map>

struct FileInfo {
    std::string path;
    std::string name;
    std::string type;
    uintmax_t size;
    std::string lastModified;
};

class FileSelector {
public:
    FileSelector();

    // Core functionality
    void scanFileSystem();
    std::vector<FileInfo> searchFiles(const std::string& query) const;
    bool addFileToRepo(const std::string& filePath, const std::string& repoPath) const;
    
    // UI methods
    void displayFileSelection(const std::vector<FileInfo>& files, int selected, const std::string& searchQuery) const;
    std::vector<FileInfo> getFilteredFiles(const std::string& searchQuery) const;

private:
    std::vector<FileInfo> fileIndex;
    std::map<std::string, std::vector<FileInfo>> fileTypeIndex;

    // Helper methods
    void indexDirectory(const std::string& path);
    std::string getFileType(const std::string& path) const;
    std::string formatSize(uintmax_t bytes) const;
    std::string formatTimestamp(const std::filesystem::file_time_type& time) const;
    bool matchesQuery(const FileInfo& file, const std::string& query) const;
    void categorizeFile(const FileInfo& file);
};

#endif // FILE_SELECTOR_H 