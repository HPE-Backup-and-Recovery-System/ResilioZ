#pragma once

#include "backup_engine.h"
#include <memory>
#include <string>

class BackupInterface {
private:
    static std::string last_backup_path;

public:
    static bool initializeBackupSystem(std::shared_ptr<BackupEngine>& engine, std::shared_ptr<Logger>& logger);
    static void backupDirectory(std::shared_ptr<BackupEngine>& engine);
    static void verifyBackup(std::shared_ptr<BackupEngine>& engine);
    static void showBackupStatus(std::shared_ptr<BackupEngine>& engine);
    static void showBackupLocation(std::shared_ptr<BackupEngine>& engine);
    static void printMenu();
    static std::string getLastBackupPath() { return last_backup_path; }
}; 