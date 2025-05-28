#include "services/backup_service.h"
#include "backup/full_backup.h"
#include <iostream>
#include <nlohmann/json.hpp>
#include <filesystem>


    // Explicit constructor and destructor definitions
    BackupService::BackupService() = default;
    BackupService::~BackupService() = default;
    namespace fs = std::filesystem;
    void BackupService::Run() {
      while (true) {
        std::cout << "\n ==== Backup Service ==== \n";
        std::cout << "\t1. Run Full Backup\n";
        std::cout << "\t2. Exit\n";
        std::cout << "\t => Enter choice: ";
    
        int choice;
        std::cin >> choice;
    
        switch (choice) {
          case 1: {
            std::string config_path, destination_path;
            std::cin.ignore();
            //std::cout << "Enter config file path: ";
            //config_path = "/home/lalithmint/Desktop/Linux-Backup-and-Recovery/config.json";
            //std::getline(std::cin, config_path);
            std::cout << "Enter destination directory path: ";
            std::getline(std::cin, destination_path);
            config_path = (fs::current_path().parent_path() / "config.json").string();
            FullBackup full_backup;
            full_backup.Execute(config_path, destination_path);
            break;
          }
          case 2:
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
    
  
    