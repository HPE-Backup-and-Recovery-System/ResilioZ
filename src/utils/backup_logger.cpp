#include "utils/backup_logger.h"

#include <fstream>
#include <stdexcept>


namespace utils { 
void BackupLogger::AppendToBackupLog(const std::string& log_file_path, const std::string& backup_filename) {
  std::ofstream ofs(log_file_path, std::ios::app);
  if (!ofs.is_open()) {
    throw std::runtime_error("Failed to open backup log file: " + log_file_path);
  }
  ofs << backup_filename << '\n';
  ofs.close();
}

}  // namespace utils


