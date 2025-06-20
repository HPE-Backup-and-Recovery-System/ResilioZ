#include "gui/dialog/create_repository_dialog.h"

#include <QString>

#include "gui/decorators/message_box.h"
#include "gui/decorators/progress_box.h"
#include "gui/dialog/ui_create_repository_dialog.h"
#include "repositories/all.h"
#include "utils/utils.h"

CreateRepositoryDialog::CreateRepositoryDialog(QWidget* parent)
    : QDialog(parent), ui(new Ui::CreateRepositoryDialog) {
  ui->setupUi(this);

  ui->stackedWidget_createRepo->setCurrentIndex(0);
  ui->backButton->setAutoDefault(true);
  ui->backButton->setDefault(false);
  ui->nextButton->setAutoDefault(true);
  ui->nextButton->setDefault(true);

  connect(ui->stackedWidget_createRepo, &QStackedWidget::currentChanged, this,
          &CreateRepositoryDialog::updateButtons);

  updateButtons();
  repository_ = nullptr;
  repodata_mgr_ = new RepodataManager();
}

CreateRepositoryDialog::~CreateRepositoryDialog() {
  delete ui;
  delete repodata_mgr_;
  if (repository_) delete repository_;
}

void CreateRepositoryDialog::setRepository(Repository* repository) {
  repository_ = repository;
}

Repository* CreateRepositoryDialog::getRepository() const {
  return repository_;
}

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
    ui->nextButton->setEnabled(false);
    initRepository();
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
      MessageBoxDecorator::showMessageBox(this, "Invalid Input",
                                          "Repository name is invalid.",
                                          QMessageBox::Warning);
      return false;
    }
    path_ = ui->localPathInput->text();
    if (path_.isEmpty()) {
      path_ = ".";
    }
    if (!Validator::IsValidPath(path_.toStdString())) {
      MessageBoxDecorator::showMessageBox(
          this, "Invalid Input", "Path for local repository is invalid.",
          QMessageBox::Warning);
      return false;
    }
  } else if (type_ == "nfs") {
    name_ = ui->nfsNameInput->text();
    if (!Validator::IsValidRepoName(name_.toStdString())) {
      MessageBoxDecorator::showMessageBox(this, "Invalid Input",
                                          "Repository name is invalid.",
                                          QMessageBox::Warning);
      return false;
    }
    path_ = ui->nfsMountPathInput->text();
    if (!Validator::IsValidPath(path_.toStdString())) {
      MessageBoxDecorator::showMessageBox(this, "Invalid Input",
                                          "NFS mount path is invalid.",
                                          QMessageBox::Warning);
      return false;
    }
  } else if (type_ == "remote") {
    name_ = ui->remoteNameInput->text();
    if (!Validator::IsValidRepoName(name_.toStdString())) {
      MessageBoxDecorator::showMessageBox(this, "Invalid Input",
                                          "Repository name is invalid.",
                                          QMessageBox::Warning);
      return false;
    }
    path_ = ui->remotePathInput->text();
    if (!Validator::IsValidSftpPath(path_.toStdString())) {
      MessageBoxDecorator::showMessageBox(
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
    MessageBoxDecorator::showMessageBox(
        this, "Invalid Password", "Password is invalid.", QMessageBox::Warning);
    return false;
  }

  QString password_confirm = ui->passwordConfirmInput->text();
  if (password_ != password_confirm) {
    MessageBoxDecorator::showMessageBox(this, "Invalid Password",
                                        "Passwords do not match.",
                                        QMessageBox::Warning);
    return false;
  }
  return true;
}

void CreateRepositoryDialog::initRepository() {
  timestamp_ = TimeUtil::GetCurrentTimestamp();

  if (type_ == "local") {
    setRepository(new LocalRepository(path_.toStdString(), name_.toStdString(),
                                      password_.toStdString(), timestamp_));
  } else if (type_ == "nfs") {
    setRepository(new NFSRepository(path_.toStdString(), name_.toStdString(),
                                    password_.toStdString(), timestamp_));
  } else if (type_ == "remote") {
    setRepository(new RemoteRepository(path_.toStdString(), name_.toStdString(),
                                       password_.toStdString(), timestamp_));
  } else {
    MessageBoxDecorator::showMessageBox(
        this, "Failure", "Could not create repository due to invalid type.",
        QMessageBox::Warning);

    Logger::SystemLog("GUI | Repository creation failure due to invalid type.",
                      LogLevel::ERROR);
    return;
  }

  ProgressBoxDecorator::runProgressBoxIndeterminate(
      this,
      [&](std::function<void(const QString&)> setWaitMessage,
          std::function<void(const QString&)> setSuccessMessage,
          std::function<void(const QString&)> setFailureMessage) -> bool {
        try {
          setWaitMessage("Checking if repository already exists...");
          if (repository_->Exists()) {
            setFailureMessage("Repository already exists at location: " +
                              QString::fromStdString(repository_->GetPath()));
            return false;
          }

          setWaitMessage("Initializing repository...");
          repository_->Initialize();

          Logger::SystemLog(
              "GUI | Repository: " + name_.toStdString() + " [" +
              RepodataManager::GetFormattedTypeString(repository_->GetType()) +
              "] created at location: " + repository_->GetPath());

          setSuccessMessage(
              "Repository: " + name_ + " [" +
              QString::fromStdString(RepodataManager::GetFormattedTypeString(
                  repository_->GetType())) +
              "] created at location: " +
              QString::fromStdString(repository_->GetPath()));
          return true;

        } catch (const std::exception& e) {
          Logger::SystemLog(
              "GUI | Cannot initialize repository: " + std::string(e.what()),
              LogLevel::ERROR);

          setFailureMessage("Repository creation failed: " +
                            QString::fromStdString(e.what()));
          return false;
        }
      },
      "Creating repository...", "Repository created successfully.",
      "Repository creation failed.",
      [&](bool success) {
        if (success) {
          repodata_mgr_->AddEntry({repository_->GetName(),
                                   repository_->GetPath(), type_.toStdString(),
                                   repository_->GetHashedPassword(),
                                   timestamp_});
          accept();
        } else {
          ui->nextButton->setEnabled(true);
        }
      });
}
