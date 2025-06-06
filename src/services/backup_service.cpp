#include "services/backup_service.h"

#include <filesystem>
#include <fstream>
#include <iostream>
#include <nlohmann/json.hpp>
#include "utils/backup_utils.h"
#include "backup/differential_backup.h"
#include "backup/full_backup.h"

namespace services {

BackupService::BackupService() = default;
BackupService::~BackupService() = default;

namespace fs = std::filesystem;
using bu = utils::BackupUtils;
void BackupService::Run() {
  while (true) {
    std::cout << "\n ==== Backup Service ==== \n";
    std::cout << "\t1. Run Full Backup\n";
    std::cout << "\t2. Run differential backup\n";
    std::cout << "\t3. Exit\n";
    std::cout << "\t => Enter choice: ";

    int choice;
    std::cin >> choice;

    switch (choice) {
      case 1: {
        std::string config_path, destination_path;
        std::cin.ignore();
        std::cout << "Enter destination directory path: ";
        std::getline(std::cin, destination_path);
        config_path =
            (fs::current_path().parent_path() / "config.json").string();
        backup::FullBackup full_backup;
        full_backup.PerformBackup(config_path, destination_path);
        bu::SetDestPathIntoConfig(config_path,destination_path);
        break;
      }
      case 2: {
        std::string config_path, destination_path;
        std::cin.ignore();
        // std::cout << "Enter destination directory path: ";
        // std::getline(std::cin, destination_path);
        config_path =
            (fs::current_path().parent_path() / "config.json").string();
        destination_path = bu::GetDestPath(config_path);
        backup::DifferentialBackup diff_backup;
        diff_backup.PerformBackup(config_path, destination_path);
        break;
      }
      case 3:
        std::cout << "Exiting Backup Service...\n";
        return;
      default:
        std::cout << "Invalid choice. Try again.\n";
        break;
    }
  }
}

void BackupService::Log() {
  std::cout << "[BackupService] Logging not implemented yet.\n";
}

} // namespace services