#ifndef UTILS_BACKUP_UTILS_H_
#define UTILS_BACKUP_UTILS_H_   

#include <filesystem>
#include <string>

namespace   utils {
    class BackupUtils {
        public:
        static std::string GetSourcePathFromConfig(const std::string& config_path);
        static std::string GetCurrentTimestamp();
        static std::string GetDestPath(const std::string& config_path);
        static void SetDestPathIntoConfig(const std::string& config_path,
            const std::string& dest_path);
    };
    
}

#endif    // UTILS_BACKUP_UTILS_H