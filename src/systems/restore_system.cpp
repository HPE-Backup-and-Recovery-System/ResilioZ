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
  if (repository_ != nullptr) {
    delete repository_;
  }
}

void RestoreSystem::Run() {
  std::string title = UserIO::DisplayMaxTitle("RESTORE SYSTEM", false);

  std::vector<std::string> main_menu = {"Go BACK...", "Restore from Backup",
                                        "List Backups", "Compare Backups"};

  while (true) {
    try {
      if (repository_ != nullptr) {
        delete repository_;
        repository_ = nullptr;
      }

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
  bool loop = true;
  try {
    do {
      repository_ = repo_service_->SelectExistingRepository();
      if (repository_ != nullptr) {
        loop = false;
      }
      break;
    } while (loop);

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
    std::string timestamp = TimeUtil::GetCurrentTimestamp();
    if (location_choice == 0) {
      restore_dir = "/";
    } else {
      // Get custom restore destination path
      std::string restore_path =
          Prompter::PromptLocalPath("Path to Restore Destination");
      // Create a restore directory with timestamp
      restore_dir = fs::path(restore_path) / ("restore_" + timestamp);
    }

    fs::create_directories(restore_dir);
    restore.RestoreAll(restore_dir.string(), backup_name);

    if (location_choice == 0) {
      Logger::Log("Restore completed successfully to original location");
    } else {
      Logger::Log("Restore completed successfully to: " + restore_dir.string());
    }

  } catch (...) {
    ErrorUtil::ThrowNested("Restore operation failed");
  }
}

void RestoreSystem::ListBackups() {
  UserIO::DisplayMaxTitle("Fetch Backups of Repository");
  bool loop = true;
  try {
    do {
      repository_ = repo_service_->SelectExistingRepository();
      if (repository_ != nullptr) {
        loop = false;
      }
      break;
    } while (loop);
    Restore restore(repository_);
    // List available backups
    std::vector<std::string> backups = restore.ListBackups();

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
  UserIO::DisplayMaxTitle("Compare Backups of Repository");
  bool loop = true;
  try {
    do {
      repo_service_->ListRepositories();
      repository_ = repo_service_->SelectExistingRepository();
      if (repository_ != nullptr) {
        loop = false;
      }
      break;
    } while (loop);
    Restore restore(repository_);
    // List available backups
    std::vector<std::string> backups = restore.ListBackups();

    std::vector<std::string> menu = {"Go BACK..."};
    menu.insert(menu.end(), backups.begin(), backups.end());

    int choice1 = UserIO::HandleMenuWithSelect(
        UserIO::DisplayMinTitle("Select First Backup", false), menu);
    if (choice1 == 0) return;

    int choice2 = UserIO::HandleMenuWithSelect(
        UserIO::DisplayMinTitle("Select Second Backup", false), menu);
    if (choice2 == 0) return;

    restore.CompareBackups(backups[choice1 - 1], backups[choice2 - 1]);

  } catch (...) {
    ErrorUtil::ThrowNested("Backup comparison failure");
  }
}

// void RestoreSystem::ResumeFailedRestore() {
//   try {
//     // Get the restore directory from user
//     std::string restore_path = Prompter::PromptPath("Enter Path to Failed
//     Restore Directory"); fs::path restore_dir(restore_path);

//     // Check if dumps directory exists
//     fs::path dumps_dir = restore_dir / "dumps";
//     if (!fs::exists(dumps_dir)) {
//       ErrorUtil::ThrowError("No dumps directory found. This may not be a
//       failed restore directory.");
//     }

//     // Check if failed files log exists
//     fs::path failed_log_path = dumps_dir / "failed_restores.log";
//     if (!fs::exists(failed_log_path)) {
//       ErrorUtil::ThrowError("No failed restores log found. This may not be a
//       failed restore directory.");
//     }

//     // Read the failed files log to get the list of failed files
//     std::vector<std::string> failed_files;
//     std::ifstream failed_log(failed_log_path);
//     std::string line;
//     while (std::getline(failed_log, line)) {
//       if (line.find("Failed to restore: ") == 0) {
//         failed_files.push_back(line.substr(19)); // Remove "Failed to
//         restore: " prefix
//       }
//     }

//     if (failed_files.empty()) {
//       ErrorUtil::ThrowError("No failed files found in the log.");
//     }

//     // Get the backup path and name from the successful restores log
//     fs::path success_log_path = dumps_dir / "successful_files" /
//     "successful_restores.log"; std::string backup_path, backup_name; if
//     (fs::exists(success_log_path)) {
//       std::ifstream success_log(success_log_path);
//       std::string line;
//       while (std::getline(success_log, line)) {
//         if (line.find("Backup path: ") == 0) {
//           backup_path = line.substr(13);
//         } else if (line.find("Backup name: ") == 0) {
//           backup_name = line.substr(12);
//         }
//       }
//     }

//     if (backup_path.empty() || backup_name.empty()) {
//       ErrorUtil::ThrowError("Could not determine backup information from
//       logs.");
//     }

//     // Create a new restore operation for the failed files
//     Restore restore_op(backup_path, restore_dir.string(), backup_name);

//     // Create new dumps directory for this resume attempt
//     std::string timestamp = TimeUtil::GetCurrentTimestamp();
//     fs::path new_dumps_dir = restore_dir / ("dumps_resume_" + timestamp);
//     fs::path new_success_dir = new_dumps_dir / "successful_files";
//     fs::create_directories(new_dumps_dir);
//     fs::create_directories(new_success_dir);

//     // Create new log files
//     std::ofstream new_failed_log(new_dumps_dir / "failed_restores.log");
//     std::ofstream new_success_log(new_success_dir /
//     "successful_restores.log"); new_failed_log << "Resume Failed Restores Log
//     - " << timestamp << "\n\n"; new_success_log << "Resume Successful
//     Restores Log - " << timestamp << "\n\n";

//     // Attempt to restore only the failed files
//     std::vector<std::string> still_failed_files;
//     std::vector<std::string> newly_successful_files;
//     for (const auto& filename : failed_files) {
//       try {
//         restore_op.RestoreFile(filename);
//         newly_successful_files.push_back(filename);
//         new_success_log << "Successfully restored: " << filename << "\n\n";
//       } catch (const std::exception& e) {
//         still_failed_files.push_back(filename);
//         new_failed_log << "Failed to restore: " << filename << "\n";
//         new_failed_log << "Error: " << e.what() << "\n\n";
//         Logger::Log("Failed to restore file: " + filename + " - " +
//         e.what());
//       }
//     }

//     // Log summary
//     if (still_failed_files.empty()) {
//       Logger::Log("Resume restore completed successfully. All previously
//       failed files have been restored.");
//       // Clean up both old and new dumps directories
//       fs::remove_all(dumps_dir);
//       fs::remove_all(new_dumps_dir);
//     } else {
//       std::string summary = "Resume restore completed with " +
//       std::to_string(still_failed_files.size()) +
//                            " still failed files and " +
//                            std::to_string(newly_successful_files.size()) + "
//                            newly successful files.\n" + "See logs in dumps
//                            directory: " + new_dumps_dir.string();
//       Logger::Log(summary);
//     }

//   } catch (...) {
//     ErrorUtil::ThrowNested("Resume restore operation failed");
//   }
// }

void RestoreSystem::Log() {
  Logger::TerminalLog("Restore system is up and running... \n");
}
