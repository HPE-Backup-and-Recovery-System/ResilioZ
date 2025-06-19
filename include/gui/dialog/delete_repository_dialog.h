#ifndef DELETE_REPOSITORY_DIALOG_H
#define DELETE_REPOSITORY_DIALOG_H

#include <QDialog>

#include "repositories/repository.h"
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
  Repository *repository_;
  RepodataManager *repodata_mgr_;
  std::vector<RepoEntry> repos;

  void resizeEvent(QResizeEvent *event) override;
  void checkSelection();

  void setColSize(int tableWidth);
  void fillTable();

  void deleteRepository(int row);
};

#endif  // DELETE_REPOSITORY_DIALOG_H
