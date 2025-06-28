#include "gui/decorators/core.h"

#include <QWidget>
#include <set>

#include "gui/decorators/message_box.h"
#include "gui/decorators/progress_box.h"
#include "utils/utils.h"

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
          size_t total_steps = 0;

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
