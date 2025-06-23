#include "systems/backup_system.h"

#include <iostream>
#include <string>
#include <vector>

#include "backup_restore/all.h"
#include "services/all.h"
#include "utils/utils.h"

BackupSystem::BackupSystem() {
  repo_service_ = new RepositoryService();
  scheduler_service_ = new SchedulerService();
}

BackupSystem::~BackupSystem() {
  delete repo_service_;
  delete scheduler_service_;
}

void BackupSystem::Run() {
  std::string title = UserIO::DisplayMaxTitle("BACKUP SYSTEM", false);
  std::vector<std::string> main_menu = {"Go BACK...", "Create Backup",
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
          CreateBackup();
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
      ErrorUtil::LogException(e, "Backup System");
    }
  }
}

void BackupSystem::CreateBackup() {
  std::string source, destination = "temp", remarks = "";
  BackupType type;
  std::vector<std::string> menu;
  int choice;

  bool loop = true;
  try {
    do {
      menu = {"Go BACK... ", "Create New Repository",
              "Use Existing Repository"};
      choice = UserIO::HandleMenuWithSelect(
          UserIO::DisplayMinTitle("Repository for Backup", false), menu);

      switch (choice) {
        case 0:
          std::cout << " - Going Back...\n";
          return;
        case 1: {
          loop = !repo_service_->CreateNewRepository();
          repository_ = repo_service_->GetRepository();
          break;
        }
        case 2: {
          repo_service_->ListRepositories();
          repository_ = repo_service_->SelectExistingRepository();
          if (repository_ != nullptr) {
            loop = false;
          }
          break;
        }
        default:
          Logger::TerminalLog("Menu Mismatch...", LogLevel::ERROR);
      }
    } while (loop);

    bool new_repo = choice == 1;
    source = Prompter::PromptPath("Path to Backup Source");

    menu = {"Full Backup", "Incremental Backup", "Differential Backup"};
    choice = UserIO::HandleMenuWithSelect(
        UserIO::DisplayMinTitle("Backup Type", false), menu);
    type = new_repo ? BackupType::FULL : static_cast<BackupType>(choice);
    remarks = Prompter::PromptInput("Remarks for Backup (Optional)");

    if (repository_->GetType() != RepositoryType::REMOTE && repository_->GetType() != RepositoryType::NFS) {
      destination = repository_->GetFullPath();
    }

    Backup backup(source, destination, type, remarks);
    backup.BackupDirectory();

    if (repository_->GetType() == RepositoryType::REMOTE) {
      auto* remote_repo = dynamic_cast<RemoteRepository*>(repository_);
      if (remote_repo) {
        remote_repo->UploadDirectory("temp");
        fs::remove_all("temp");
      }
    } else if (repository_->GetType() == RepositoryType::NFS) {
      auto* nfs_repo = dynamic_cast<NFSRepository*>(repository_);
      if (nfs_repo) {
        nfs_repo->UploadDirectory("temp");
        fs::remove_all("temp");
      }
    }

    menu = {"Yes", "No"};
    choice = UserIO::HandleMenuWithSelect(
        UserIO::DisplayMinTitle("Automate Backup?", false), menu);

    if (choice == 0) {
      ScheduleBackup();
    }

    Logger::Log("Backup creation success");

  } catch (...) {
    ErrorUtil::ThrowNested("Backup creation failure");
  }
}

void BackupSystem::ListBackups() {
  UserIO::DisplayMaxTitle("Fetch Backups of Path");
  std::string source = Prompter::PromptPath();

  try {
    Backup backup(source, "", BackupType::FULL, "");
    backup.DisplayAllBackupDetails(source);
  } catch (...) {
    ErrorUtil::ThrowNested("Backup listing failure");
  }
}

void BackupSystem::CompareBackups() {
  UserIO::DisplayMaxTitle("Compare Backups of Path");
  std::string source = Prompter::PromptPath();

  std::string first_backup = Prompter::PromptInput("1st Backup Name");
  std::string second_backup = Prompter::PromptInput("2nd Backup Name");

  try {
    Backup backup(source, "", BackupType::FULL, "");
    backup.CompareBackups(source, first_backup, second_backup);
  } catch (...) {
    ErrorUtil::ThrowNested("Backup comparison failure");
  }
}

void BackupSystem::ScheduleBackup() {
  // TODO: Schedule Backup
}

void BackupSystem::Log() {
  Logger::TerminalLog("Backup system is up and running... \n");
}
