#ifndef BACKUP_LOGGER_H
#define BACKUP_LOGGER_H

#include <filesystem>
#include <string>


class BackupLogger {
 public:

     static void AppendToBackupLog(const std::string& log_file_path, const std::string& backup_filename);
   };


 
#endif  // BACKUP_LOGGER_H
