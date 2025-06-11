#include "systems/restore_system.h"

#include <iostream>
#include <string>
#include <vector>
#include <filesystem>

#include "backup_restore/all.h"
#include "utils/utils.h"

namespace fs = std::filesystem;

RestoreSystem::RestoreSystem() {
  repo_service_ = new RepositoryService();
}

RestoreSystem::~RestoreSystem() {
  delete repo_service_;
}

void RestoreSystem::Run() {
  std::string title = UserIO::DisplayMaxTitle("RESTORE SYSTEM", false);
  std::vector<std::string> main_menu = {"Go BACK...", "Restore from Backup",
                                       "List Backups of Path",
                                       "Compare Backups of Path"};
  while (true) {
    try {
      int choice = UserIO::HandleMenuWithSelect(title, main_menu);

      switch (choice) {
        case 0:
          std::cout << " - Going Back...\n";
          return;
        case 1:
          RestoreFromBackup();
          break;
        case 2:
          ListBackups();
          break;
        case 3:
          CompareBackups();
          break;
        default:
          Logger::TerminalLog("Menu Mismatch...", LogLevel::ERROR);
      }
    } catch (const std::exception& e) {
      ErrorUtil::LogException(e, "Restore System");
    }
  }
}

void RestoreSystem::RestoreFromBackup() {
  try {
    // Show available repositories
    repo_service_->ListRepositories();
    
    // Get backup source path
    std::string backup_path = Prompter::PromptPath("Path to Backup Source");
    
    // Get restore destination path
    std::string restore_path = Prompter::PromptPath("Path to Restore Destination");

    // Create a restore directory with timestamp
    std::string timestamp = TimeUtil::GetCurrentTimestamp();
    fs::path restore_dir = fs::path(restore_path) / ("restore_" + timestamp);
    fs::create_directories(restore_dir);

    // List available backups
    std::vector<std::string> backups;
    fs::path backup_dir = fs::path(backup_path) / "backup";
    if (!fs::exists(backup_dir)) {
      ErrorUtil::ThrowError("No backup directory found at: " + backup_path);
    }

    for (const auto& entry : fs::directory_iterator(backup_dir)) {
      if (entry.is_regular_file()) {
        backups.push_back(entry.path().filename().string());
      }
    }

    if (backups.empty()) {
      ErrorUtil::ThrowError("No backups found in the specified path");
    }

    std::sort(backups.begin(), backups.end());

    std::vector<std::string> menu = {"Go BACK..."};
    menu.insert(menu.end(), backups.begin(), backups.end());
    int choice = UserIO::HandleMenuWithSelect(
        UserIO::DisplayMinTitle("Select Backup to Restore", false), menu);

    if (choice == 0) {
      std::cout << " - Going Back...\n";
      return;
    }

    std::string backup_name = backups[choice - 1];
    Restore restore_op(backup_path, restore_dir.string(), backup_name);
    restore_op.RestoreAll();

    Logger::Log("Restore completed successfully to: " + restore_dir.string());

  } catch (...) {
    ErrorUtil::ThrowNested("Restore operation failed");
  }
}

void RestoreSystem::ListBackups() {
  UserIO::DisplayMaxTitle("Fetch Backups of Path");
  std::string backup_path = Prompter::PromptPath();

  try {
    fs::path backup_dir = fs::path(backup_path) / "backup";
    if (!fs::exists(backup_dir)) {
      Logger::TerminalLog("No backup directory found at: " + backup_path);
      return;
    }

    std::vector<std::string> backups;
    for (const auto& entry : fs::directory_iterator(backup_dir)) {
      if (entry.is_regular_file()) {
        backups.push_back(entry.path().filename().string());
      }
    }

    if (backups.empty()) {
      Logger::TerminalLog("No backups found in the specified path");
      return;
    }

    std::sort(backups.begin(), backups.end());

    std::ostringstream backup_list;
    backup_list << "Available Backups:\n";
    for (const auto& backup : backups) {
      backup_list << " - " << backup << "\n";
    }
    Logger::TerminalLog(backup_list.str());

  } catch (...) {
    ErrorUtil::ThrowNested("Backup listing failure");
  }
}

void RestoreSystem::CompareBackups() {
  UserIO::DisplayMaxTitle("Compare Backups of Path");
  std::string backup_path = Prompter::PromptPath();

  try {
    fs::path backup_dir = fs::path(backup_path) / "backup";
    if (!fs::exists(backup_dir)) {
      Logger::TerminalLog("No backup directory found at: " + backup_path);
      return;
    }

    std::vector<std::string> backups;
    for (const auto& entry : fs::directory_iterator(backup_dir)) {
      if (entry.is_regular_file()) {
        backups.push_back(entry.path().filename().string());
      }
    }

    if (backups.size() < 2) {
      Logger::TerminalLog("Need at least two backups to compare");
      return;
    }

    std::sort(backups.begin(), backups.end());

    std::vector<std::string> menu = {"Go BACK..."};
    menu.insert(menu.end(), backups.begin(), backups.end());

    int choice1 = UserIO::HandleMenuWithSelect(
        UserIO::DisplayMinTitle("Select First Backup", false), menu);
    if (choice1 == 0) return;

    int choice2 = UserIO::HandleMenuWithSelect(
        UserIO::DisplayMinTitle("Select Second Backup", false), menu);
    if (choice2 == 0) return;

    Restore restore(backup_path, "", backups[choice1 - 1]);
    restore.CompareBackups(backups[choice1 - 1], backups[choice2 - 1]);

  } catch (...) {
    ErrorUtil::ThrowNested("Backup comparison failure");
  }
}

void RestoreSystem::Log() {
  Logger::TerminalLog("Restore system is up and running... \n");
}
