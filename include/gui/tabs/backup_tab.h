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
  void on_nextButton_clicked();
  void on_backButton_clicked();
  void on_nextButton_2_clicked();
  void on_backButton_2_clicked();
  void on_nextButton_3_clicked();
  void on_backButton_3_clicked();

  void on_createBackupButton_clicked();
  void on_createRepoButton_clicked();
  void on_useRepoButton_clicked();

  void on_listBackupButton_clicked();
  void on_chooseRepoButton_list_clicked();

  void on_compareBackupButton_clicked();
  void on_chooseRepoButton_compare_clicked();

 private:
  Ui::BackupTab* ui;
  Repository* repository_;
  Backup* backup_;
  BackupType backup_type_ = BackupType::FULL;
  std::string source_path_, remarks_ = "";

  void resizeEvent(QResizeEvent* event) override;
  void checkRepoSelection();
  void checkBackupSelection();

  void setColSize(int tableWidth);
  void fillListTable();
  void fillCompareTable();

  void updateProgress();
  void updateButtons();
  void checkSelection();

  bool handleSelectRepo();
  bool handleBackupDetails();
  bool handleSchedule();
  bool handleSelectBackups();

  void initBackup();
  void listBackups();
  void compareBackups();
};

#endif  // BACKUP_TAB_H
