#ifndef USE_REPOSITORY_DIALOG_H
#define USE_REPOSITORY_DIALOG_H

#include <QDialog>

#include "repositories/repository.h"
#include "utils/repodata_manager.h"

namespace Ui {
class UseRepositoryDialog;
}

class UseRepositoryDialog : public QDialog {
  Q_OBJECT

 public:
  explicit UseRepositoryDialog(QWidget *parent = nullptr);
  ~UseRepositoryDialog();

  void setRepository(Repository *repository);
  Repository *getRepository() const;

 private slots:
  void on_backButton_clicked();
  void on_nextButton_clicked();

 private:
  Ui::UseRepositoryDialog *ui;
  Repository *repository_;
  RepodataManager *repodata_mgr_;
  std::vector<RepoEntry> repos;
  void resizeEvent(QResizeEvent *event) override;
  void checkSelection();

  void setColSize(int tableWidth);
  void fillTable();

  void useRepository(int row);
};

#endif  // USE_REPOSITORY_DIALOG_H
