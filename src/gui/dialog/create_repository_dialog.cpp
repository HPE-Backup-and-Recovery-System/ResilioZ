#include "gui/dialog/create_repository_dialog.h"

#include <QString>

#include "gui/decorators/message_box.h"
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
    MessageBoxDecorator::ShowMessageBox(this, "Success",
                                        "Repository created." + type_,
                                        QMessageBox::Information);
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
      MessageBoxDecorator::ShowMessageBox(this, "Invalid Input",
                                          "Repository name is invalid.",
                                          QMessageBox::Warning);
      return false;
    }
    path_ = ui->localPathInput->text();
    if (!Validator::IsValidPath(path_.toStdString())) {
      MessageBoxDecorator::ShowMessageBox(
          this, "Invalid Input", "Path for local repository is invalid.",
          QMessageBox::Warning);
      return false;
    }
  } else if (type_ == "nfs") {
    name_ = ui->nfsNameInput->text();
    if (!Validator::IsValidRepoName(name_.toStdString())) {
      MessageBoxDecorator::ShowMessageBox(this, "Invalid Input",
                                          "Repository name is invalid.",
                                          QMessageBox::Warning);
      return false;
    }
    server_ip_ = ui->nfsServerIpInput->text();
    if (!Validator::IsValidIpAddress(server_ip_.toStdString())) {
      MessageBoxDecorator::ShowMessageBox(this, "Invalid Input",
                                          "NFS server IP address is invalid.",
                                          QMessageBox::Warning);
      return false;
    }
    server_backup_path_ = ui->nfsServerBackupPathInput->text();
    if (!Validator::IsValidLocalPath(server_backup_path_.toStdString())) {
      MessageBoxDecorator::ShowMessageBox(this, "Invalid Input",
                                          "NFS server backup path is invalid.",
                                          QMessageBox::Warning);
      return false;
    }
    client_mount_path_ = ui->nfsClientMountPathInput->text();
    if (!Validator::IsValidMountPath(client_mount_path_.toStdString())) {
      MessageBoxDecorator::ShowMessageBox(this, "Invalid Input",
                                          "NFS client mount path is invalid.",
                                          QMessageBox::Warning);
      return false;
    }
  } else if (type_ == "remote") {
    name_ = ui->remoteNameInput->text();
    if (!Validator::IsValidRepoName(name_.toStdString())) {
      MessageBoxDecorator::ShowMessageBox(this, "Invalid Input",
                                          "Repository name is invalid.",
                                          QMessageBox::Warning);
      return false;
    }
    path_ = ui->remotePathInput->text();
    if (!Validator::IsValidSftpPath(path_.toStdString())) {
      MessageBoxDecorator::ShowMessageBox(
          this, "Invalid Input", "Path for remote repository is invalid.",
          QMessageBox::Warning);
      return false;
    }
  }
  return true;
}

bool CreateRepositoryDialog::handleSetPassword() {
  password_ = ui->passwordInput->text();
  if (!Validator::IsValidPassword(password_.toStdString())) {
    MessageBoxDecorator::ShowMessageBox(
        this, "Invalid Password", "Password is invalid.", QMessageBox::Warning);
    return false;
  }

  QString password_confirm = ui->passwordConfirmInput->text();
  if (password_ != password_confirm) {
    MessageBoxDecorator::ShowMessageBox(this, "Invalid Password",
                                        "Passwords do not match.",
                                        QMessageBox::Warning);
    return false;
  }
  return true;
}
