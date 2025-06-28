#include "systems/backup_system.h"

#include <iostream>
#include <string>
#include <vector>

#include "backup_restore/all.h"
#include "repositories/all.h"
#include "services/all.h"
#include "utils/utils.h"

BackupSystem::BackupSystem() {
  repo_service_ = new RepositoryService();
  scheduler_service_ = new SchedulerService();
}

BackupSystem::~BackupSystem() {
  delete repo_service_;
  delete scheduler_service_;
  if (repository_) delete repository_;
}

void BackupSystem::Run() {
  std::string title = UserIO::DisplayMaxTitle("BACKUP SYSTEM", false);
  std::vector<std::string> main_menu = {"Go BACK...", "Create Backup",
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
  std::string source, destination, remarks = "";
  BackupType type;
  std::vector<std::string> menu;
  int choice;

  bool loop = true;
  try {
    do {
      if (repository_ != nullptr) {
        delete repository_;
        repository_ = nullptr;
      }

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

    if (repository_->GetType() != RepositoryType::REMOTE &&
        repository_->GetType() != RepositoryType::NFS) {
      destination = repository_->GetFullPath();
    }

    Backup backup(repository_, source, type, remarks);
    backup.BackupDirectory();

    menu = {"Yes", "No"};
    choice = UserIO::HandleMenuWithSelect(
        UserIO::DisplayMinTitle("Automate Backup?", false), menu);

    if (choice == 0) {
      ScheduleBackup(source, destination, type, remarks);
    }

    Logger::Log("Backup creation success");

  } catch (...) {
    ErrorUtil::ThrowNested("Backup creation failure");
  }
}

void BackupSystem::ListBackups() {
  UserIO::DisplayMaxTitle("Fetch Backups of Repository");
  std::string source = "/";
  bool loop = true;
  try {
    do {
      repository_ = repo_service_->SelectExistingRepository();
      if (repository_ != nullptr) {
        loop = false;
      }
      break;
    } while (loop);
    if (repository_ != nullptr) {
      Backup backup(repository_, source, BackupType::FULL, "");
      backup.DisplayAllBackupDetails();
    }
  } catch (...) {
    ErrorUtil::ThrowNested("Backup listing failure");
  }
}

void BackupSystem::CompareBackups() {
  UserIO::DisplayMaxTitle("Compare Backups of Repository");
  std::string source = "/";
  bool loop = true;
  try {
    do {
      repository_ = repo_service_->SelectExistingRepository();
      if (repository_ != nullptr) {
        loop = false;
      }
      break;
    } while (loop);
    if (repository_ != nullptr) {
      Backup backup(repository_, source, BackupType::FULL, "");
      std::vector<std::string> backups = backup.ListBackups();
      std::vector<std::string> menu = {"Go BACK..."};
      menu.insert(menu.end(), backups.begin(), backups.end());

      int choice1 = UserIO::HandleMenuWithSelect(
          UserIO::DisplayMinTitle("Select First Backup", false), menu);
      if (choice1 == 0) return;

      int choice2 = UserIO::HandleMenuWithSelect(
          UserIO::DisplayMinTitle("Select Second Backup", false), menu);
      if (choice2 == 0) return;
      backup.CompareBackups(backups[choice1 - 1], backups[choice2 - 1]);
    }
  } catch (...) {
    ErrorUtil::ThrowNested("Backup comparison failure");
  }
}

void BackupSystem::ScheduleBackup(std::string source, std::string destination,
                                  BackupType type, std::string remarks) {
  std::string destination_name = repository_->GetName();
  std::string destination_path = repository_->GetPath();
  std::string destination_password = repository_->GetPassword();
  RepositoryType destination_type = repository_->GetType();
  std::string destination_created_at = "";

  scheduler_service_->AttachSchedule(
      source, destination_name, destination_path, destination_password,
      destination_created_at, destination_type, type, remarks);
}

void BackupSystem::Log() {
  Logger::TerminalLog("Backup system is up and running... \n");
}
