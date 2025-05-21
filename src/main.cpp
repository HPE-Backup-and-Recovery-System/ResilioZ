#include "backup_interface.h"
#include <iostream>
#include <memory>

/*
// Test case function for demonstration purposes
void run_test_case() {
    std::shared_ptr<BackupEngine> engine;
    std::shared_ptr<Logger> logger;
    
    // Initialize the backup system
    if (!BackupInterface::initializeBackupSystem(engine, logger)) {
        std::cout << "Failed to initialize backup system for test case\n";
        return;
    }
    
    // Test backup of a sample directory
    const std::string test_dir = "/tmp/backup_test";
    std::filesystem::create_directories(test_dir);
    
    // Create some test files
    std::ofstream file1(test_dir + "/test1.txt");
    file1 << "Test data 1" << std::endl;
    file1.close();
    
    std::ofstream file2(test_dir + "/test2.txt");
    file2 << "Test data 2" << std::endl;
    file2.close();
    
    // Perform backup
    std::cout << "\n=== Running Test Case ===\n";
    std::cout << "1. Testing backup of: " << test_dir << "\n";
    BackupInterface::backupDirectory(engine);
    
    // Verify backup
    std::cout << "\n2. Verifying backup\n";
    BackupInterface::verifyBackup(engine);
    
    // Show status
    std::cout << "\n3. Checking backup status\n";
    BackupInterface::showBackupStatus(engine);
    
    // Cleanup
    std::filesystem::remove_all(test_dir);
    engine->cleanup();
    
    std::cout << "\n=== Test Case Completed ===\n";
}
*/

int main() {
    std::shared_ptr<BackupEngine> engine;
    std::shared_ptr<Logger> logger;
    int choice;

    while (true) {
        BackupInterface::printMenu();
        std::cin >> choice;
        std::cin.ignore(); // Clear the newline character

        switch (choice) {
            case 1:
                if (BackupInterface::initializeBackupSystem(engine, logger)) {
                    std::cout << "System ready for backup operations.\n";
                }
                break;
            case 2:
                BackupInterface::backupDirectory(engine);
                break;
            case 3:
                BackupInterface::verifyBackup(engine);
                break;
            case 4:
                BackupInterface::showBackupStatus(engine);
                break;
            case 5:
                BackupInterface::showBackupLocation(engine);
                break;
            case 6:
                if (engine) {
                    engine->cleanup();
                }
                std::cout << "Exiting...\n";
                return 0;
            default:
                std::cout << "Invalid choice. Please try again.\n";
        }
    }

    return 0;
}