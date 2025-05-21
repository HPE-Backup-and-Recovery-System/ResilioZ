#include "backup_interface.h"
#include <iostream>
#include <filesystem>
#include <chrono>
#include <pwd.h>
#include <unistd.h>

namespace fs = std::filesystem;

// Initialize static member
std::string BackupInterface::last_backup_path;

std::string expandHomePath(const std::string& path) {
    if (path.empty() || path[0] != '~') {
        return path;
    }
    
    const char* home = getenv("HOME");
    if (!home) {
        struct passwd* pwd = getpwuid(getuid());
        if (pwd) {
            home = pwd->pw_dir;
        }
    }
    
    if (!home) {
        return path;
    }
    
    return std::string(home) + path.substr(1);
}

void BackupInterface::printMenu() {
    std::cout << "\n=== NFS-NAS Backup System ===\n";
    std::cout << "1. Initialize Backup System\n";
    std::cout << "2. Backup Directory\n";
    std::cout << "3. Verify Last Backup\n";
    std::cout << "4. Show Backup Status\n";
    std::cout << "5. Show Backup Location\n";
    std::cout << "6. Exit\n";
    std::cout << "Enter your choice (1-6): ";
}

bool BackupInterface::initializeBackupSystem(std::shared_ptr<BackupEngine>& engine, std::shared_ptr<Logger>& logger) {
    try {
        fs::path config_path = fs::current_path() / "config.json";
        if (!fs::exists(config_path)) {
            std::cout << "Error: config.json not found in current directory.\n";
            return false;
        }

        logger = std::make_shared<Logger>();
        engine = std::make_shared<BackupEngine>(config_path.string(), logger);
        
        if (!engine->initialize()) {
            std::cout << "Failed to initialize backup system.\n";
            return false;
        }

        std::cout << "Backup system initialized successfully!\n";
        return true;
    } catch (const std::exception& e) {
        std::cout << "Error initializing backup system: " << e.what() << "\n";
        return false;
    }
}

void BackupInterface::backupDirectory(std::shared_ptr<BackupEngine>& engine) {
    if (!engine) {
        std::cout << "Error: Backup system not initialized.\n";
        return;
    }

    std::string source_path;
    std::cout << "Enter the full path of the directory to backup (e.g., ~/Downloads or /home/username/Downloads): ";
    std::getline(std::cin >> std::ws, source_path);

    // Expand ~ to home directory
    source_path = expandHomePath(source_path);

    if (!fs::exists(source_path)) {
        std::cout << "Error: Directory does not exist: " << source_path << "\n";
        std::cout << "Please check that:\n";
        std::cout << "1. The path is correct\n";
        std::cout << "2. You have permission to access the directory\n";
        std::cout << "3. The directory exists\n";
        return;
    }

    if (!fs::is_directory(source_path)) {
        std::cout << "Error: Path is not a directory: " << source_path << "\n";
        return;
    }

    std::cout << "Starting backup of: " << source_path << "\n";
    if (engine->performBackup(source_path)) {
        std::cout << "Backup completed successfully!\n";
        last_backup_path = source_path;  // Store the path of the last backup
    } else {
        std::cout << "Backup failed.\n";
    }
}

void BackupInterface::verifyBackup(std::shared_ptr<BackupEngine>& engine) {
    if (!engine) {
        std::cout << "Error: Backup system not initialized.\n";
        return;
    }

    std::string source_path;
    if (!last_backup_path.empty()) {
        std::cout << "Last backed up directory was: " << last_backup_path << "\n";
        std::cout << "Press Enter to verify this directory, or enter a different path: ";
        std::string input;
        std::getline(std::cin, input);
        
        if (input.empty()) {
            source_path = last_backup_path;
        } else {
            source_path = input;
        }
    } else {
        std::cout << "Enter the full path of the directory to verify (e.g., ~/Downloads or /home/username/Downloads): ";
        std::getline(std::cin >> std::ws, source_path);
    }

    // Expand ~ to home directory
    source_path = expandHomePath(source_path);

    if (!fs::exists(source_path)) {
        std::cout << "Error: Directory does not exist: " << source_path << "\n";
        std::cout << "Please check that:\n";
        std::cout << "1. The path is correct\n";
        std::cout << "2. You have permission to access the directory\n";
        std::cout << "3. The directory exists\n";
        return;
    }

    if (!fs::is_directory(source_path)) {
        std::cout << "Error: Path is not a directory: " << source_path << "\n";
        return;
    }

    std::cout << "Verifying backup of: " << source_path << "\n";
    if (engine->verifyBackup(source_path)) {
        std::cout << "Backup verification successful!\n";
    } else {
        std::cout << "Backup verification failed.\n";
    }
}

void BackupInterface::showBackupLocation(std::shared_ptr<BackupEngine>& engine) {
    if (!engine) {
        std::cout << "Error: Backup system not initialized.\n";
        return;
    }

    const json& config = engine->getConfig();
    std::string mount_point = config["mount_point"].get<std::string>();
    
    std::cout << "\nBackup Location Information:\n";
    std::cout << "NFS Server: " << config["nfs_server"].get<std::string>() << "\n";
    std::cout << "Mount Point: " << mount_point << "\n\n";

    if (!fs::exists(mount_point)) {
        std::cout << "Error: Backup location does not exist. The NFS share might not be mounted.\n";
        return;
    }

    std::cout << "Contents of backup location:\n";
    try {
        for (const auto& entry : fs::directory_iterator(mount_point)) {
            std::cout << (entry.is_directory() ? "📁 " : "📄 ") 
                      << entry.path().filename().string() << "\n";
        }
    } catch (const fs::filesystem_error& e) {
        std::cout << "Error accessing backup location: " << e.what() << "\n";
        std::cout << "Please check that:\n";
        std::cout << "1. The NFS share is properly mounted\n";
        std::cout << "2. You have permission to access the directory\n";
    }
}

void BackupInterface::showBackupStatus(std::shared_ptr<BackupEngine>& engine) {
    if (!engine) {
        std::cout << "Error: Backup system not initialized.\n";
        return;
    }

    auto last_backup = engine->getLastBackupTime();
    auto now = std::chrono::system_clock::now();
    auto duration = std::chrono::duration_cast<std::chrono::hours>(now - last_backup);

    std::cout << "\nBackup Status:\n";
    std::cout << "Last backup: " << std::chrono::system_clock::to_time_t(last_backup) << "\n";
    std::cout << "Hours since last backup: " << duration.count() << "\n";
    std::cout << "Retention policy: " << engine->getRetentionDays() << " days\n";
    if (!last_backup_path.empty()) {
        std::cout << "Last backed up directory: " << last_backup_path << "\n";
    }
} 