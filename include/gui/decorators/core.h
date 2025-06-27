#ifndef CORE_DECORATOR_H
#define CORE_DECORATOR_H

#include <QWidget>

#include "backup_restore/all.h"
#include "repositories/all.h"

class BackupGUI : public Backup {
 public:
  BackupGUI(QWidget* parent, Repository* repo, const fs::path& input_path,
            BackupType type = BackupType::FULL,
            const std::string& remarks = "");
  ~BackupGUI();

  void BackupDirectory(std::function<void(bool)> onFinishCallback = nullptr);

 private:
  QWidget* parent_;
};

#endif  // CORE_DECORATOR_H