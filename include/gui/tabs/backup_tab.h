#ifndef BACKUP_TAB_H
#define BACKUP_TAB_H

#include <QWidget>

#include "gui/decorators/core.h"
#include "repositories/repository.h"
#include "services/all.h"
#include "systems/system.h"
#include "utils/scheduler_request_manager.h"

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

  void on_yesSchButton_clicked();

 private:
  Ui::BackupTab* ui;
  Repository* repository_;
  BackupGUI* backup_;
  BackupType backup_type_ = BackupType::FULL;
  std::vector<BackupDetails> backup_list_;
  std::string source_path_, remarks_ = "";
  std::string cmp_first_backup_ = "", cmp_second_backup_ = "";
  SchedulerRequestManager* request_mgr;

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
