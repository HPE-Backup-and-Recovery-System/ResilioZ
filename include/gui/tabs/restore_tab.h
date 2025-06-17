#ifndef RESTORE_TAB_H
#define RESTORE_TAB_H

#include <QWidget>
#include "repositories/repository.h"
#include "services/all.h"
#include "systems/system.h"

namespace Ui {
class RestoreTab;
}

class RestoreTab : public QWidget {
  Q_OBJECT

 public:
  explicit RestoreTab(QWidget *parent = nullptr);
  ~RestoreTab();

private slots:
    void on_restoreButton_clicked();
    void on_retryButton_clicked();

    void on_nextButton_clicked();
    void on_backButton_clicked();
    void on_chooseRepoButton_clicked();

private:
  Ui::RestoreTab *ui;

  Repository* repository_;
  RepositoryService* repo_service_;

  void updateProgress();
  void updateButtons();
};

#endif  // RESTORE_TAB_H
