#include "systems/restore_system.h"

#include <filesystem>
#include <fstream>
#include <iostream>
#include <string>
#include <vector>

#include "backup_restore/all.h"
#include "utils/utils.h"
#include "utils/validator.h"

namespace fs = std::filesystem;

RestoreSystem::RestoreSystem() {
  repo_service_ = new RepositoryService();
  repository_ = nullptr;
}

RestoreSystem::~RestoreSystem() {
  delete repo_service_;
  if (repository_ != nullptr) delete repository_;
}

void RestoreSystem::Run() {
  std::string title = UserIO::DisplayMaxTitle("RESTORE SYSTEM", false);

  std::vector<std::string> main_menu = {"Go BACK...", "Restore from Backup",
                                        "Verify Backup"};

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
          VerifyBackup();
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
    repository_ = repo_service_->SelectExistingRepository();
    if (repository_ == nullptr) {
      return;
    }

    Restore restore(repository_);
    // List available backups
    std::vector<std::string> backups = restore.ListBackups();
    std::vector<std::string> menu = {"Go BACK..."};
    menu.insert(menu.end(), backups.begin(), backups.end());
    int choice = UserIO::HandleMenuWithSelect(
        UserIO::DisplayMinTitle("Select Backup to Restore", false), menu);

    if (choice == 0) {
      std::cout << " - Going Back...\n";
      return;
    }

    std::string backup_name = backups[choice - 1];

    // Ask if user wants to restore to original location
    std::vector<std::string> location_options = {"Original location",
                                                 "Custom location"};
    int location_choice = UserIO::HandleMenuWithSelect(
        UserIO::DisplayMinTitle("Select Restore Location", false),
        location_options);

    fs::path restore_dir;
    if (location_choice == 0) {
      restore_dir = "/";
    } else {
      // Get custom restore destination path
      std::string restore_path =
          Prompter::PromptLocalPath("Path to Restore Destination");
      // Create a restore directory with timestamp
      restore_dir = fs::path(restore_path);
    }

    fs::create_directories(restore_dir);
    restore.RestoreAll(restore_dir.string(), backup_name);

  } catch (...) {
    ErrorUtil::ThrowNested("Restore operation failed");
  }
}

void RestoreSystem::VerifyBackup() {
  try {
    repository_ = repo_service_->SelectExistingRepository();
    if (repository_ == nullptr) {
      return;
    }

    Restore restore(repository_);
    // List available backups
    std::vector<std::string> backups = restore.ListBackups();
    std::vector<std::string> menu = {"Go BACK..."};
    menu.insert(menu.end(), backups.begin(), backups.end());
    int choice = UserIO::HandleMenuWithSelect(
        UserIO::DisplayMinTitle("Select Backup to Restore", false), menu);

    if (choice == 0) {
      std::cout << " - Going Back...\n";
      return;
    }

    std::string backup_name = backups[choice - 1];

    restore.VerifyBackup(backup_name);

  } catch (...) {
    ErrorUtil::ThrowNested("Backup verification operation failed");
  }
}



void RestoreSystem::Log() {
  Logger::TerminalLog("Restore system is up and running... \n");
}
