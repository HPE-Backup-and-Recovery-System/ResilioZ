#include "systems/restore_system.h"

#include <iostream>
#include <string>
#include <vector>
#include <filesystem>
#include <fstream>

#include "backup_restore/all.h"
#include "utils/utils.h"
#include "utils/validator.h"

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
                                       "Compare Backups of Path",
                                       "Resume Failed Restore"};
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
        // case 4:
        //   ResumeFailedRestore();
        //   break;
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
    // Get list of available repositories
    const auto repos = repo_service_->GetAllRepositories();
    if (repos.empty()) {
      ErrorUtil::ThrowError("No repositories found. Please create a repository first.");
    }

    // Create repository selection menu
    std::vector<std::string> repo_menu = {"Go BACK..."};
    for (const auto& repo : repos) {
      repo_menu.push_back(repo.name + " [" + RepodataManager::GetFormattedTypeString(repo.type) + "] - " + repo.path);
    }

    // Let user select repository
    int repo_choice = UserIO::HandleMenuWithSelect(
        UserIO::DisplayMinTitle("Select Repository", false), repo_menu);
 
    if (repo_choice == 0) {
      std::cout << " - Going Back...\n";
      return;
      }

    // Get selected repository path 
    std::string backup_path = repos[repo_choice - 1].path + "/" + repos[repo_choice - 1].name;

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
    
    // Ask if user wants to restore to original location
    std::vector<std::string> location_options = {
        "Restore to original location",
        "Restore to custom location"
    };
    int location_choice = UserIO::HandleMenuWithSelect(
        UserIO::DisplayMinTitle("Select Restore Location", false), location_options);

    fs::path restore_dir;
    std::string timestamp = TimeUtil::GetCurrentTimestamp();
    if (location_choice == 0) {
      restore_dir = "/";
    }
    else {
      // Get custom restore destination path
      std::string restore_path = Prompter::PromptPath("Enter Path to Restore Destination");
      if (!Validator::IsValidPath(restore_path)) {
        ErrorUtil::ThrowError("Invalid restore path format");
      }
      // Create a restore directory with timestamp
      restore_dir = fs::path(restore_path) / ("restore_" + timestamp);
    }
  
    fs::create_directories(restore_dir);

    // Create the actual restore operation

    Logger::Log(restore_dir.string());
    Restore restore_op(backup_path, restore_dir.string(), backup_name);
    restore_op.RestoreAll();
    
    // // Create dumps directory for logging
    // fs::path dumps_dir = restore_dir / "dumps";
    // fs::create_directories(dumps_dir);
    
    // // Create log files
    // std::ofstream failed_log(dumps_dir / "failed_restores.log");
    // std::ofstream success_log(dumps_dir / "successful_restores.log");
    // failed_log << "Failed Restores Log - " << TimeUtil::GetCurrentTimestamp() << "\n\n";
    // success_log << "Successful Restores Log - " << TimeUtil::GetCurrentTimestamp() << "\n\n";

    // // Restore all files and track failures
    // std::vector<std::string> failed_files;
    // for (const auto& [file_path, metadata] : restore_op.GetMetadata().files) {
    //   bool restore_success = false;
    //   std::string error_message;

    //   // Check if all chunks exist
    //   std::vector<std::string> missing_chunks;
    //   for (const auto& chunk_hash : metadata.chunk_hashes) {
    //     std::string subdir = chunk_hash.substr(0, 2);
    //     fs::path chunk_path = fs::path(backup_path) / "chunks" / subdir / (chunk_hash + ".chunk");
    //     if (!fs::exists(chunk_path)) {
    //       missing_chunks.push_back(chunk_hash);
    //     }
    //   }

    //   if (!missing_chunks.empty()) {
    //     failed_log << "Skipped restore (missing chunks): " << metadata.original_filename << "\n";
    //     failed_log << "Original path: " << file_path << "\n";
    //     failed_log << "Missing chunks: " << missing_chunks.size() << "\n";
    //     failed_log << "Missing chunk hashes: " << std::accumulate(
    //         std::next(missing_chunks.begin()), missing_chunks.end(),
    //         missing_chunks[0],
    //         [](const std::string& a, const std::string& b) { return a + ", " + b; }) << "\n\n";
    //     failed_files.push_back(metadata.original_filename);
    //     Logger::Log("Skipped restore of " + metadata.original_filename + " due to missing chunks");
    //     continue;
    //   }

    //   // Try to restore the file up to 2 times
    //   for (int attempt = 1; attempt <= 2 && !restore_success; attempt++) {
    //     try {
    //       restore_op.RestoreFile(metadata.original_filename);
    //       restore_success = true;
    //       success_log << "Successfully restored: " << metadata.original_filename << "\n";
    //       success_log << "Original path: " << file_path << "\n\n";
    //     } catch (const std::exception& e) {
    //       error_message = e.what();
    //       if (attempt == 1) {
    //         Logger::Log("First attempt failed for file: " + metadata.original_filename + " - Retrying...");
    //       }
    //     }
    //   }

    //   if (!restore_success) {
    //     failed_files.push_back(metadata.original_filename);
    //     failed_log << "Failed to restore: " << metadata.original_filename << "\n";
    //     failed_log << "Original path: " << file_path << "\n";
    //     failed_log << "Error: " << error_message << "\n";
    //     failed_log << "Attempts: 2\n\n";
    //     Logger::Log("Failed to restore file after 2 attempts: " + metadata.original_filename + " - " + error_message);
    //   }
    // }

    // // Log summary
    // if (failed_files.empty()) {
    if (location_choice == 0) {
      Logger::Log("Restore completed successfully to original locations");
    } else {
      Logger::Log("Restore completed successfully to: " + restore_dir.string());
      }
    // } else {
    //   std::string summary = "Restore operation was unsuccessful. " + 
    //                        std::to_string(failed_files.size()) + " files failed to restore.\n" +
    //                        "Failed files have been logged in: " + dumps_dir.string() + "\n" +
    //                        "Use the 'Resume Failed Restore' option to retry failed files.";
    //   Logger::Log(summary);
    // }

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
      if (entry.is_regular_file()) {if (!Validator::IsValidPath(backup_path)) {
      ErrorUtil::ThrowError("Invalid backup path format");
    }
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

// void RestoreSystem::ResumeFailedRestore() {
//   try {
//     // Get the restore directory from user
//     std::string restore_path = Prompter::PromptPath("Enter Path to Failed Restore Directory");
//     fs::path restore_dir(restore_path);
    
//     // Check if dumps directory exists
//     fs::path dumps_dir = restore_dir / "dumps";
//     if (!fs::exists(dumps_dir)) {
//       ErrorUtil::ThrowError("No dumps directory found. This may not be a failed restore directory.");
//     }

//     // Check if failed files log exists
//     fs::path failed_log_path = dumps_dir / "failed_restores.log";
//     if (!fs::exists(failed_log_path)) {
//       ErrorUtil::ThrowError("No failed restores log found. This may not be a failed restore directory.");
//     }

//     // Read the failed files log to get the list of failed files
//     std::vector<std::string> failed_files;
//     std::ifstream failed_log(failed_log_path);
//     std::string line;
//     while (std::getline(failed_log, line)) {
//       if (line.find("Failed to restore: ") == 0) {
//         failed_files.push_back(line.substr(19)); // Remove "Failed to restore: " prefix
//       }
//     }

//     if (failed_files.empty()) {
//       ErrorUtil::ThrowError("No failed files found in the log.");
//     }

//     // Get the backup path and name from the successful restores log
//     fs::path success_log_path = dumps_dir / "successful_files" / "successful_restores.log";
//     std::string backup_path, backup_name;
//     if (fs::exists(success_log_path)) {
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
//       ErrorUtil::ThrowError("Could not determine backup information from logs.");
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
//     std::ofstream new_success_log(new_success_dir / "successful_restores.log");
//     new_failed_log << "Resume Failed Restores Log - " << timestamp << "\n\n";
//     new_success_log << "Resume Successful Restores Log - " << timestamp << "\n\n";

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
//         Logger::Log("Failed to restore file: " + filename + " - " + e.what());
//       }
//     }

//     // Log summary
//     if (still_failed_files.empty()) {
//       Logger::Log("Resume restore completed successfully. All previously failed files have been restored.");
//       // Clean up both old and new dumps directories
//       fs::remove_all(dumps_dir);
//       fs::remove_all(new_dumps_dir);
//     } else {
//       std::string summary = "Resume restore completed with " + std::to_string(still_failed_files.size()) + 
//                            " still failed files and " + std::to_string(newly_successful_files.size()) +
//                            " newly successful files.\n" +
//                            "See logs in dumps directory: " + new_dumps_dir.string();
//       Logger::Log(summary);
//     }

//   } catch (...) {
//     ErrorUtil::ThrowNested("Resume restore operation failed");
//   }
// }

void RestoreSystem::Log() {
  Logger::TerminalLog("Restore system is up and running... \n");
}
