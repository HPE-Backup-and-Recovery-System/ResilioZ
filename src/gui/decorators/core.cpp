#include "gui/decorators/core.h"

#include <QWidget>
#include <set>

#include "gui/decorators/message_box.h"
#include "gui/decorators/progress_box.h"
#include "utils/utils.h"

RestoreGUI::RestoreGUI(QWidget* parent, Repository* repo)
    : Restore(repo), parent_(parent) {}

RestoreGUI::~RestoreGUI() {}

void RestoreGUI::RestoreAll(std::function<void(bool)> onFinishCallback,
                            const fs::path output_path_,
                            const std::string backup_name_) {
  ProgressBoxDecorator::runProgressBoxDeterminate(
      parent_,
      [this, output_path_, backup_name_](
          std::function<void(int)> setProgress,
          std::function<void(const QString&)> setWaitMessage,
          std::function<void(const QString&)> setSuccessMessage,
          std::function<void(const QString&)> setFailureMessage) -> bool {
        try {
          setWaitMessage("Checking if repository exists...");
          try {
            if (!repo_->Exists()) {
              setFailureMessage("Repository does not exist at location: " +
                                QString::fromStdString(repo_->GetPath()));
              Logger::SystemLog("GUI | Restore | Failed to find repository",
                                LogLevel::ERROR);
              return false;
            }
          } catch (const std::exception& e) {
            Logger::SystemLog("GUI | Restore | Failed to initialize restore: " +
                                  std::string(e.what()),
                              LogLevel::ERROR);
            setFailureMessage("Repository existence unconfirmed");
            return false;
          }

          setWaitMessage("Checking if destination folder exists...");
          try {
            QFileInfo destInfo(QString::fromStdString(output_path_));
            if (!destInfo.exists() || !destInfo.isDir()) {
              setFailureMessage(
                  "Destination folder does not exist at location: " +
                  QString::fromStdString(output_path_));

              Logger::SystemLog("GUI | Restore | Failed to find destination",
                                LogLevel::ERROR);
              return false;
            }
          } catch (const std::exception& e) {
            Logger::SystemLog("GUI | Failed to verify restore destination: " +
                                  std::string(e.what()),
                              LogLevel::ERROR);

            setFailureMessage("Destination existence unconfirmed");
            return false;
          }

          QString message = QString::fromStdString(
              "Attempting restore from " + repo_->GetRepositoryInfoString() +
              " File: " + backup_name_);
          setWaitMessage(message);

          setWaitMessage("Preparing Destination...");
          if (!fs::exists(output_path_)) {
            try {
              fs::create_directories(output_path_);
              setWaitMessage(
                  QString::fromStdString(output_path_.string() + " created"));
            } catch (const std::filesystem::filesystem_error& e) {
              setFailureMessage(QString::fromStdString(
                  "Failed to create directory: " + std::string(e.what())));
              return false;
            }
          } else {
            setWaitMessage(
                QString::fromStdString(output_path_.string() + " exists"));
          }

          try {
            LoadMetadata(backup_name_);
          } catch (const std::exception& e) {
            setFailureMessage(QString::fromStdString(
                "Backup metadata not found: " + std::string(e.what())));
            return false;
          }

          int processed_files = 0;
          int restored_files = 0;

          if (!metadata_ || !(*metadata_)) {
            setFailureMessage("Metadata unavailable for backup.");
            Logger::SystemLog("GUI | Restore | Metadata pointer is null",
                              LogLevel::ERROR);
            return false;
          }

          int total_files = (*metadata_)->files.size();

          if (total_files == 0) {
            setFailureMessage("No files to restore in the selected backup.");
            Logger::SystemLog("GUI | Restore | No files in backup metadata",
                              LogLevel::WARNING);
            return false;
          }

          std::vector<std::string> failed_files;
          for (const auto& [file_path, metadata] : (*metadata_)->files) {
            try {
              setWaitMessage(QString::fromStdString("Restoring " + file_path));
              RestoreFile(file_path, output_path_, backup_name_);
              restored_files++;
              setWaitMessage(
                  QString::fromStdString("Restore success for " + file_path));
            } catch (const std::exception& e) {
              setWaitMessage(
                  QString::fromStdString("Restore failed for " + file_path));
              Logger::SystemLog(
                  "GUI | Restore | Failed to restore: " + file_path,
                  LogLevel::ERROR);
              failed_files.push_back(file_path);
            }

            processed_files++;
            setProgress(
                static_cast<int>((processed_files * 100) / total_files));
          }

          std::ostringstream out;
          out << "Restore Summary:\n"
              << " - Total files: " << total_files << "\n"
              << " - Processed files: " << processed_files << "\n"
              << " - Restored files: " << restored_files << "\n";

          if (total_files != restored_files) {
            out << " - Status: Restore incomplete\n";

            out << " - Missing files \n";
            for (std::string file_name_ : failed_files) {
              out << " --  " << file_name_ << " \n";
            }
            setFailureMessage(QString::fromStdString(out.str()));
            return false;
          }

          out << " - Status: Restore OK\n";
          setSuccessMessage(QString::fromStdString(out.str()));

          return true;

        } catch (const std::exception& e) {
          Logger::SystemLog(
              "GUI | Restore | Cannot restore: " + std::string(e.what()),
              LogLevel::ERROR);

          setFailureMessage("Restore operation failed: " +
                            QString::fromStdString(e.what()));
          return false;
        }
      },
      "Attempting restore...", "Restore operation successful.",
      "Restore operation failed.",
      [this, onFinishCallback](bool success) {
        if (onFinishCallback) {
          onFinishCallback(success);
        }
      });
}

BackupGUI::BackupGUI(QWidget* parent, Repository* repo,
                     const fs::path& input_path, BackupType type,
                     const std::string& remarks)
    : Backup(repo, input_path, type, remarks), parent_(parent) {}

BackupGUI::~BackupGUI() {}

void BackupGUI::BackupDirectory(std::function<void(bool)> onFinishCallback) {
  ProgressBoxDecorator::runProgressBoxDeterminate(
      parent_,
      [this](std::function<void(int)> setProgress,
             std::function<void(const QString&)> setWaitMessage,
             std::function<void(const QString&)> setSuccessMessage,
             std::function<void(const QString&)> setFailureMessage) -> bool {
        try {
          setWaitMessage("Scanning files...");

          // Collect all files to backup
          std::vector<fs::path> files_to_backup;
          std::set<std::string> current_files;

          for (const auto& entry :
               fs::recursive_directory_iterator(input_path_)) {
            if (entry.is_regular_file() || fs::is_symlink(entry.path())) {
              files_to_backup.push_back(entry.path());
              current_files.insert(entry.path().string());
            }
          }

          size_t total_files = files_to_backup.size();
          size_t processed = 0;
          size_t changed_files = 0;
          size_t unchanged_files = 0;
          size_t added_files = 0;
          size_t deleted_files = 0;

          // Check deleted files
          for (auto it = metadata_.files.begin();
               it != metadata_.files.end();) {
            if (!fs::exists(it->first)) {
              deleted_files++;
              it = metadata_.files.erase(it);
            } else {
              ++it;
            }
          }

          setWaitMessage("Preparing Backup...");

          for (const auto& file_path : files_to_backup) {
            auto str_path = file_path.string();
            auto it = metadata_.files.find(str_path);

            if (it == metadata_.files.end()) {
              added_files++;
              BackupFile(file_path);
              setWaitMessage("Backing up file: " +
                             QString::fromStdString(str_path));
            } else if (CheckFileForChanges(file_path, it->second)) {
              changed_files++;
              BackupFile(file_path);
              setWaitMessage("Backing up file: " +
                             QString::fromStdString(str_path));
            } else {
              unchanged_files++;
            }

            processed++;
            setProgress(static_cast<int>((processed * 100) / total_files));
          }

          setWaitMessage("Saving metadata...");
          SaveMetadata();

          std::ostringstream out;
          out << "Backup Summary:\n"
              << " - Changed files: " << changed_files << "\n"
              << " - Unchanged files: " << unchanged_files << "\n"
              << " - Added files: " << added_files << "\n"
              << " - Deleted files: " << deleted_files;

          Logger::SystemLog("GUI | Backup created successfully.");
          setSuccessMessage(QString::fromStdString(out.str()));
          return true;

        } catch (const std::exception& e) {
          Logger::SystemLog(
              "GUI | Cannot create backup: " + std::string(e.what()),
              LogLevel::ERROR);

          setFailureMessage("Backup creation failed: " +
                            QString::fromStdString(e.what()));
          return false;
        }
      },
      "Creating backup...", "Backup created successfully.",
      "Backup creation failed.",
      [this, onFinishCallback](bool success) {
        if (onFinishCallback) {
          onFinishCallback(success);
        }
      });
}

std::string BackupGUI::CompareBackups(std::string first_backup,
                                      std::string second_backup) {
  BackupMetadata metadata1 = LoadPreviousMetadata(first_backup);
  BackupMetadata metadata2 = LoadPreviousMetadata(second_backup);

  size_t changed_files = 0;
  size_t unchanged_files = 0;
  size_t added_files = 0;
  size_t deleted_files = 0;

  // Compare files
  for (const auto& [file_path, file_metadata2] : metadata2.files) {
    auto it = metadata1.files.find(file_path);
    if (it == metadata1.files.end()) {
      added_files++;
    } else if (file_metadata2.total_size != it->second.total_size ||
               file_metadata2.mtime != it->second.mtime) {
      changed_files++;
    } else {
      unchanged_files++;
    }
  }

  // Count deleted files
  for (const auto& [file_path, _] : metadata1.files) {
    if (metadata2.files.find(file_path) == metadata2.files.end()) {
      deleted_files++;
    }
  }

  std::ostringstream comparison;
  comparison << "Backup Comparison (" << first_backup << " vs " << second_backup
             << "):"
             << "\n - Changed files: " << changed_files
             << "\n - Unchanged files: " << unchanged_files
             << "\n - Added files: " << added_files
             << "\n - Deleted files: " << deleted_files << std::endl;
  return comparison.str();
}
