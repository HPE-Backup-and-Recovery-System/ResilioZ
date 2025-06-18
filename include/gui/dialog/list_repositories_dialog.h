#ifndef LIST_REPOSITORIES_DIALOG_H
#define LIST_REPOSITORIES_DIALOG_H

#include <QDialog>

#include "utils/repodata_manager.h"

namespace Ui {
class ListRepositoriesDialog;
}

class ListRepositoriesDialog : public QDialog {
  Q_OBJECT

 public:
  explicit ListRepositoriesDialog(QWidget *parent = nullptr);
  ~ListRepositoriesDialog();

 private slots:
  void on_backButton_clicked();

 private:
  Ui::ListRepositoriesDialog *ui;
  RepodataManager *repodata_mgr_;
  std::vector<RepoEntry> repos;
  void resizeEvent(QResizeEvent *event) override;

  void SetColSize(int tableWidth);
  void FillTable();
};

#endif  // LIST_REPOSITORIES_DIALOG_H
