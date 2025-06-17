#ifndef BACKUP_TAB_H
#define BACKUP_TAB_H

#include <QWidget>

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
  void updateProgress();
  void updateButtons();

  Ui::BackupTab* ui;

  Repository* repository_;
  RepositoryService* repo_service_;
  SchedulerService* scheduler_service_;
};

#endif  // BACKUP_TAB_H
