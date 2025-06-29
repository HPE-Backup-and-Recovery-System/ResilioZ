#ifndef CORE_DECORATOR_H
#define CORE_DECORATOR_H

#include <QFileInfo>
#include <QWidget>

#include "backup_restore/all.h"
#include "repositories/all.h"

class RestoreGUI : public Restore {
 public:
  RestoreGUI(QWidget* parent, Repository* repo);

  void RestoreAll(std::function<void(bool)> onFinishCallback = nullptr,
                  const fs::path output_path_ = "",
                  const std::string backup_name_ = "");
  void VerifyBackup(std::function<void(bool)> onFinishCallback = nullptr,
                    const std::string backup_name_ = "");

  std::vector<std::string> GetSuccessFiles() const;
  std::vector<std::string> GetCorruptFiles() const;
  std::vector<std::string> GetFailedFiles() const;

  RestoreSummary GetRestoreSummary();
  void SetRestoreSummary();

 private:
  QWidget* parent_;
  RestoreSummary summary_;
};

class BackupGUI : public Backup {
 public:
  BackupGUI(QWidget* parent, Repository* repo, const fs::path& input_path,
            BackupType type = BackupType::FULL,
            const std::string& remarks = "");

  void BackupDirectory(std::function<void(bool)> onFinishCallback = nullptr);
  std::string CompareBackups(std::string first_backup,
                             std::string second_backup);

  std::vector<std::string> GetSuccessFiles() const;
  std::vector<std::string> GetFailedFiles() const;

 private:
  QWidget* parent_;
  std::vector<std::string> success_files_;
  std::vector<std::string> failed_files_;
};

#endif  // CORE_DECORATOR_H