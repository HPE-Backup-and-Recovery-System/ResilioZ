#ifndef CREATE_REPOSITORY_DIALOG_H
#define CREATE_REPOSITORY_DIALOG_H

#include <QDialog>
#include <QString>

#include "repositories/repository.h"
#include "utils/repodata_manager.h"

namespace Ui {
class CreateRepositoryDialog;
}

class CreateRepositoryDialog : public QDialog {
  Q_OBJECT

 public:
  explicit CreateRepositoryDialog(QWidget* parent = nullptr);
  ~CreateRepositoryDialog();

  void setRepository(Repository* repository);
  Repository* getRepository() const;

 private slots:
  void on_nextButton_clicked();
  void on_backButton_clicked();

 private:
  QString type_ = "local";
  QString name_, path_;
  QString password_ = "";
  std::string timestamp_;

  Ui::CreateRepositoryDialog* ui;
  Repository* repository_;
  RepodataManager* repodata_mgr_;

  void updateProgress();
  void updateButtons();

  bool handleRepoType();
  bool handleRepoDetails();
  bool handleSetPassword();

  void initRepository();
};

#endif  // CREATE_REPOSITORY_DIALOG_H
