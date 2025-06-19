#ifndef BACKUP_TAB_H
#define BACKUP_TAB_H

#include <QWidget>

#include "backup_restore/backup.hpp"
#include "repositories/repository.h"
#include "services/all.h"
#include "systems/system.h"

namespace Ui {
class BackupTab;
}

class BackupTab : public QWidget {
  Q_OBJECT

 public:
  explicit BackupTab(QWidget* parent = nullptr);
  ~BackupTab();

 private slots:
  void on_createBackupButton_clicked();
  void on_nextButton_clicked();
  void on_backButton_clicked();

  void on_createRepoButton_clicked();
  void on_useRepoButton_clicked();

 private:
  Ui::BackupTab* ui;
  Repository* repository_;
  Backup* backup_;
  BackupType backup_type_ = BackupType::FULL;
  std::string source_path_, destination_path_ = "temp", remarks_ = "";

  void updateProgress();
  void updateButtons();

  bool handleSelectRepo();
  bool handleBackupDetails();
  bool handleSchedule();

  void initBackup();
};

#endif  // BACKUP_TAB_H
