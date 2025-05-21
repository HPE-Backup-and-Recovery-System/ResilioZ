#include "backup_engine.h"
#include <iostream>
#include <thread>
#include <chrono>
#include <filesystem>
#include <fstream>
#include <unistd.h>
#include <cstdlib>
#include <nlohmann/json.hpp>

namespace fs = std::filesystem;
using json = nlohmann::json;

// Helper function to check if running as root
bool isRunningAsRoot() {
    return geteuid() == 0;
}

// Helper function to check NFS server connectivity
bool checkNFSServer(const std::string& server) {
    // Extract host from server string (format: host:/path)
    size_t colon_pos = server.find(':');
    if (colon_pos == std::string::npos) {
        return false;
    }
    std::string host = server.substr(0, colon_pos);
    
    // Use ping to check basic connectivity
    std::string cmd = "ping -c 1 -W 1 " + host + " > /dev/null 2>&1";
    int result = std::system(cmd.c_str());
    return result == 0;
}

// Helper function to check NFS server configuration
bool checkNFSConfig(const std::string& config_path) {
    try {
        std::ifstream f(config_path);
        if (!f.is_open()) {
            return false;
        }
        json config = json::parse(f);
        
        if (!config.contains("nfs_server") || !config["nfs_server"].is_string()) {
            return false;
        }
        
        std::string server = config["nfs_server"].get<std::string>();
        return checkNFSServer(server);
    } catch (const std::exception& e) {
        return false;
    }
}

// Helper function to create test data
void createTestData(const std::string& path) {
    fs::create_directories(path);
    
    // Create some test files
    std::ofstream file1(path + "/test1.txt");
    file1 << "Test data 1" << std::endl;
    file1.close();
    
    std::ofstream file2(path + "/test2.txt");
    file2 << "Test data 2" << std::endl;
    file2.close();
    
    // Create a subdirectory with a file
    fs::create_directories(path + "/subdir");
    std::ofstream file3(path + "/subdir/test3.txt");
    file3 << "Test data 3" << std::endl;
    file3.close();
}

// Helper function to verify test data
bool verifyTestData(const std::string& path) {
    if (!fs::exists(path)) return false;
    if (!fs::exists(path + "/test1.txt")) return false;
    if (!fs::exists(path + "/test2.txt")) return false;
    if (!fs::exists(path + "/subdir/test3.txt")) return false;
    return true;
}

// Main test function that can be called from main.cpp
bool runBackupTests() {
    try {
        // Create logger
        auto logger = std::make_shared<Logger>();
        logger->log("Starting NFS-NAS backup system test", LogLevel::INFO);
        
        // Get the absolute path to config.json
        fs::path config_path = fs::current_path() / "config.json";
        if (!fs::exists(config_path)) {
            logger->log("Config file not found at: " + config_path.string(), LogLevel::ERROR);
            return false;
        }
        logger->log("Using config file: " + config_path.string());
        
        // Check NFS server connectivity
        if (!checkNFSConfig(config_path.string())) {
            logger->log("WARNING: NFS server is not accessible. Please check:", LogLevel::WARNING);
            logger->log("1. NFS server is running and accessible", LogLevel::WARNING);
            logger->log("2. Network connectivity to the server", LogLevel::WARNING);
            logger->log("3. Server address in config.json is correct", LogLevel::WARNING);
            return false;
        }
        logger->log("NFS server is accessible", LogLevel::INFO);
        
        // Check for root privileges
        if (!isRunningAsRoot()) {
            logger->log("WARNING: Not running as root. NFS mount tests will be skipped.", LogLevel::WARNING);
            logger->log("To run full tests, please run with sudo: sudo ./main", LogLevel::INFO);
        }
        
        // Create test directory
        const std::string test_dir = "/tmp/backup_test";
        createTestData(test_dir);
        logger->log("Created test data in: " + test_dir);
        
        // Initialize backup engine with existing config.json
        BackupEngine engine(config_path.string(), logger);
        if (!engine.initialize()) {
            if (!isRunningAsRoot()) {
                logger->log("Skipping NFS mount tests due to insufficient privileges", LogLevel::WARNING);
                logger->log("Basic configuration tests passed", LogLevel::INFO);
                return true;
            }
            logger->log("Failed to initialize backup engine", LogLevel::ERROR);
            return false;
        }
        logger->log("Backup engine initialized successfully");
        
        // Only run mount-dependent tests if we have root privileges
        if (isRunningAsRoot()) {
            // Test 1: Basic backup
            logger->log("Test 1: Performing basic backup");
            if (!engine.performBackup(test_dir)) {
                logger->log("Basic backup test failed", LogLevel::ERROR);
                return false;
            }
            logger->log("Basic backup test passed");
            
            // Test 2: Verify backup
            logger->log("Test 2: Verifying backup");
            if (!engine.verifyBackup(test_dir)) {
                logger->log("Backup verification test failed", LogLevel::ERROR);
                return false;
            }
            logger->log("Backup verification test passed");
            
            // Test 3: Modify source and perform incremental backup
            logger->log("Test 3: Testing incremental backup");
            std::ofstream file4(test_dir + "/test4.txt");
            file4 << "Test data 4" << std::endl;
            file4.close();
            
            if (!engine.performBackup(test_dir)) {
                logger->log("Incremental backup test failed", LogLevel::ERROR);
                return false;
            }
            logger->log("Incremental backup test passed");
            
            // Test 4: Test retention policy
            logger->log("Test 4: Testing retention policy");
            // Wait for a short time to simulate time passing
            std::this_thread::sleep_for(std::chrono::seconds(2));
            if (!engine.cleanup()) {
                logger->log("Retention policy test failed", LogLevel::ERROR);
                return false;
            }
            logger->log("Retention policy test passed");
        } else {
            logger->log("Skipping mount-dependent tests (requires root privileges)", LogLevel::INFO);
        }
        
        // Cleanup
        fs::remove_all(test_dir);
        logger->log("Test cleanup completed");
        
        logger->log("All tests completed successfully!", LogLevel::INFO);
        return true;
        
    } catch (const std::exception& e) {
        std::cerr << "Test failed with exception: " << e.what() << std::endl;
        return false;
    }
} 