#include "gui/dialog/create_repository_dialog.h"

#include <QMessageBox>
#include <QString>

#include "gui/dialog/ui_create_repository_dialog.h"
#include "utils/utils.h"

CreateRepositoryDialog::CreateRepositoryDialog(QWidget *parent)
    : QDialog(parent), ui(new Ui::CreateRepositoryDialog) {
  ui->setupUi(this);
  ui->stackedWidget_createRepo->setCurrentIndex(0);

  connect(ui->stackedWidget_createRepo, &QStackedWidget::currentChanged, this,
          &CreateRepositoryDialog::updateButtons);

  updateButtons();
}

CreateRepositoryDialog::~CreateRepositoryDialog() { delete ui; }

void CreateRepositoryDialog::updateProgress() {
  int index = ui->stackedWidget_createRepo->currentIndex();
  int total = ui->stackedWidget_createRepo->count();
  int percent = ((index + 1) * 100) / total;
  ui->progressBar->setValue(percent);
}

void CreateRepositoryDialog::updateButtons() {
  int index = ui->stackedWidget_createRepo->currentIndex();
  int total = ui->stackedWidget_createRepo->count();
  if (index == 0) {
    // ui->backButton->setEnabled(false);
  } else {
    ui->backButton->setEnabled(true);
  }
  if (index == total - 1) {
    ui->nextButton->setText("Create");
  } else {
    ui->nextButton->setText("Next");
  }
}

void CreateRepositoryDialog::on_nextButton_clicked() {
  int index = ui->stackedWidget_createRepo->currentIndex();
  int total = ui->stackedWidget_createRepo->count();

  bool valid = true;
  // TODO: Validate Inputs
  switch (index) {
    case 0:
      valid = handleRepoType();
      break;

    case 1:
      valid = handleRepoDetails();
      break;

    case 2:
      valid = handleSetPassword();
      break;

    default:
      break;
  }

  if (!valid) {
    return;
  }

  if (index < total - 1) {
    ui->stackedWidget_createRepo->setCurrentIndex(index + 1);
  } else {
    // TODO: Create Repo
    QMessageBox::information(this, "Success", "Repository created." + type_);
    accept();
  }
  updateProgress();
}

void CreateRepositoryDialog::on_backButton_clicked() {
  int index = ui->stackedWidget_createRepo->currentIndex();
  if (index > 0) {
    ui->stackedWidget_createRepo->setCurrentIndex(index - 1);
  } else {
    reject();
  }
  updateProgress();
}

bool CreateRepositoryDialog::handleRepoType() {
  if (ui->localRepoButton->isChecked()) {
    type_ = "local";
    ui->stackedWidget_repoForms->setCurrentIndex(0);
    return true;
  }
  if (ui->nfsRepoButton->isChecked()) {
    type_ = "nfs";
    ui->stackedWidget_repoForms->setCurrentIndex(2);
    return true;
  }
  if (ui->remoteRepoButton->isChecked()) {
    type_ = "remote";
    ui->stackedWidget_repoForms->setCurrentIndex(1);
    return true;
  }
  return false;
}

bool CreateRepositoryDialog::handleRepoDetails() {
  if (type_ == "local") {
    name_ = ui->localNameInput->text();
    if (!Validator::IsValidRepoName(name_.toStdString())) {
      QMessageBox::warning(this, "Warning", "Repository name is invalid.");
      return false;
    }
    path_ = ui->localPathInput->text();
    if (!Validator::IsValidPath(path_.toStdString())) {
      QMessageBox::warning(this, "Warning",
                           "Path for local repository is invalid.");
      return false;
    }
  } else if (type_ == "nfs") {
    name_ = ui->nfsNameInput->text();
    if (!Validator::IsValidRepoName(name_.toStdString())) {
      QMessageBox::warning(this, "Warning", "Repository name is invalid.");
      return false;
    }
    server_ip_ = ui->nfsServerIpInput->text();
    if (!Validator::IsValidIpAddress(server_ip_.toStdString())) {
      QMessageBox::warning(this, "Warning", "Server IP address is invalid.");
      return false;
    }
    server_backup_path_ = ui->nfsServerBackupPathInput->text();
    if (!Validator::IsValidLocalPath(server_backup_path_.toStdString())) {
      QMessageBox::warning(this, "Warning",
                           "NFS server backup path is invalid.");
      return false;
    }
    client_mount_path_ = ui->nfsClientMountPathInput->text();
    if (!Validator::IsValidMountPath(client_mount_path_.toStdString())) {
      QMessageBox::warning(this, "Warning",
                           "NFS client mount path is invalid.");
      return false;
    }
  } else if (type_ == "remote") {
    name_ = ui->remoteNameInput->text();
    if (!Validator::IsValidRepoName(name_.toStdString())) {
      QMessageBox::warning(this, "Warning", "Repository name is invalid.");
      return false;
    }
    path_ = ui->remotePathInput->text();
    if (!Validator::IsValidSftpPath(path_.toStdString())) {
      QMessageBox::warning(this, "Warning",
                           "Path for remote repository is invalid.");
      return false;
    }
  }
  return true;
}

bool CreateRepositoryDialog::handleSetPassword() {
  password_ = ui->passwordInput->text();
  if (!Validator::IsValidPassword(password_.toStdString())) {
    QMessageBox::warning(this, "Warning", "Password is invalid.");
    return false;
  }

  QString password_confirm = ui->passwordConfirmInput->text();
  if (password_ != password_confirm) {
    QMessageBox::warning(this, "Warning", "Passwords do not match.");
    return false;
  }
  return true;
}
