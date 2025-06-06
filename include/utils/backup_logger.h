#ifndef UTILS_BACKUP_LOGGER_H
#define UTILS_BACKUP_LOGGER_H

#include <filesystem>
#include <string>

namespace utils {

   /**
    * @brief A utility class for logging backup operations.
    */

class BackupLogger {
 public:

     static void AppendToBackupLog(const std::string& log_file_path, const std::string& backup_filename);
   };


 
}  // namespace utils
#endif  // UTILS_BACKUP_LOGGER_H
