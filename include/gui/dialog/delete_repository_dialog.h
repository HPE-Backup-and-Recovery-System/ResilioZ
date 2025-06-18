#ifndef DELETE_REPOSITORY_DIALOG_H
#define DELETE_REPOSITORY_DIALOG_H

#include <QDialog>

#include "utils/repodata_manager.h"

namespace Ui {
class DeleteRepositoryDialog;
}

class DeleteRepositoryDialog : public QDialog {
  Q_OBJECT

 public:
  explicit DeleteRepositoryDialog(QWidget *parent = nullptr);
  ~DeleteRepositoryDialog();

 private slots:
  void on_backButton_clicked();
  void on_nextButton_clicked();

 private:
  Ui::DeleteRepositoryDialog *ui;
  RepodataManager *repodata_mgr_;
  std::vector<RepoEntry> repos;
  void resizeEvent(QResizeEvent *event) override;
  void checkSelection();

  void SetColSize(int tableWidth);
  void FillTable();
};

#endif  // DELETE_REPOSITORY_DIALOG_H
