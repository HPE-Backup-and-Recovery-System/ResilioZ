#ifndef CREATE_REPOSITORY_DIALOG_H
#define CREATE_REPOSITORY_DIALOG_H

#include <QDialog>
#include <QString>

namespace Ui {
class CreateRepositoryDialog;
}

class CreateRepositoryDialog : public QDialog {
  Q_OBJECT

 public:
  explicit CreateRepositoryDialog(QWidget *parent = nullptr);
  ~CreateRepositoryDialog();

 private slots:
  void on_nextButton_clicked();
  void on_backButton_clicked();

 private:
  QString type_ = "local";
  QString name_, path_, server_ip_, server_backup_path_, client_mount_path_;
  QString password_ = "";

  Ui::CreateRepositoryDialog *ui;

  void updateProgress();
  void updateButtons();

  bool handleRepoType();
  bool handleRepoDetails();
  bool handleSetPassword();
};

#endif  // CREATE_REPOSITORY_DIALOG_H
