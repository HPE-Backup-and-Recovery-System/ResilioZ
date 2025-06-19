#include "gui/tabs/backup_tab.h"

#include "backup_restore/backup.hpp"
#include "gui/decorators/message_box.h"
#include "gui/decorators/progress_box.h"
#include "gui/dialog/create_repository_dialog.h"
#include "gui/dialog/use_repository_dialog.h"
#include "gui/tabs/ui_backup_tab.h"
#include "utils/utils.h"

BackupTab::BackupTab(QWidget* parent) : QWidget(parent), ui(new Ui::BackupTab) {
  ui->setupUi(this);

  ui->stackedWidget->setCurrentIndex(0);
  ui->stackedWidget_createBackup->setCurrentIndex(0);
  ui->backButton->setAutoDefault(true);
  ui->backButton->setDefault(false);
  ui->nextButton->setAutoDefault(true);
  ui->nextButton->setDefault(true);

  repository_ = nullptr;
  backup_ = nullptr;

  connect(ui->stackedWidget_createBackup, &QStackedWidget::currentChanged, this,
          &BackupTab::updateButtons);

  updateButtons();
}

BackupTab::~BackupTab() {
  delete ui;
  if (backup_) delete backup_;
  if (repository_) delete repository_;
}

void BackupTab::on_createBackupButton_clicked() {
  ui->stackedWidget->setCurrentIndex(1);
}

void BackupTab::updateProgress() {
  int index = ui->stackedWidget_createBackup->currentIndex();
  int total = ui->stackedWidget_createBackup->count();
  int percent = ((index + 1) * 100) / total;
  ui->progressBar->setValue(percent);
}

void BackupTab::updateButtons() {
  int index = ui->stackedWidget_createBackup->currentIndex();
  int total = ui->stackedWidget_createBackup->count();
  if (index == 0) {
    // ui->backButton->setEnabled(false);
  } else {
    ui->backButton->setEnabled(true);
  }
  if (index == total - 1) {
    ui->nextButton->setText("Start");
  } else {
    ui->nextButton->setText("Next");
  }
}

void BackupTab::on_backButton_clicked() {
  int index = ui->stackedWidget_createBackup->currentIndex();
  if (index > 0) {
    ui->stackedWidget_createBackup->setCurrentIndex(index - 1);
  } else {
    ui->stackedWidget->setCurrentIndex(0);
  }
  updateProgress();
}

void BackupTab::on_nextButton_clicked() {
  int index = ui->stackedWidget_createBackup->currentIndex();
  int total = ui->stackedWidget_createBackup->count();

  bool valid = true;
  switch (index) {
    case 0:
      valid = handleSelectRepo();
      break;
    case 1:
      valid = handleBackupDetails();
      break;
    case 2:
      valid = handleSchedule();
      break;
    default:
      break;
  }
  if (!valid) {
    return;
  }

  if (index < total - 1) {
    ui->stackedWidget_createBackup->setCurrentIndex(index + 1);
  } else {
    ui->nextButton->setEnabled(false);
    initBackup();
  }
  updateProgress();
}

void BackupTab::on_createRepoButton_clicked() {
  CreateRepositoryDialog dialog(this);
  dialog.setWindowFlags(Qt::Window);
  if (dialog.exec() == QDialog::Accepted) {
    repository_ = dialog.getRepository();
  } else {
    repository_ = nullptr;
  }
}

void BackupTab::on_useRepoButton_clicked() {
  UseRepositoryDialog dialog(this);
  dialog.setWindowFlags(Qt::Window);
  if (dialog.exec() == QDialog::Accepted) {
    repository_ = dialog.getRepository();
  } else {
    repository_ = nullptr;
  }
}

bool BackupTab::handleSelectRepo() {
  if (repository_ == nullptr) {
    MessageBoxDecorator::showMessageBox(
        this, "No Repository Selected",
        "Please select a repository to continue.", QMessageBox::Warning);
    return false;
  }
  return true;
}

bool BackupTab::handleBackupDetails() {
  source_path_ = ui->srcInput->text().toStdString();
  if (source_path_.empty()) {
    source_path_ = ".";
  }
  if (!Validator::IsValidLocalPath(source_path_)) {
    MessageBoxDecorator::showMessageBox(
        this, "Invalid Input", "Source path is invalid.", QMessageBox::Warning);
    return false;
  }

  destination_path_ = repository_->GetFullPath();
  remarks_ = ui->remarksInput->text().toStdString();

  if (ui->incButton->isChecked()) {
    backup_type_ = BackupType::INCREMENTAL;
  } else if (ui->diffButton->isChecked()) {
    backup_type_ = BackupType::DIFFERENTIAL;
  } else {
    backup_type_ = BackupType::FULL;
  }
  return true;
}

bool BackupTab::handleSchedule() {
  bool schedule = ui->yesSchButton->isChecked();
  if (schedule) {
    // TODO: Schedule
  }
  return true;
}

void BackupTab::initBackup() {
  if (repository_->GetType() == RepositoryType::REMOTE) {
    backup_ = new Backup(source_path_, "temp", backup_type_, remarks_);
  } else {
    backup_ =
        new Backup(source_path_, destination_path_, backup_type_, remarks_);
  }

  // TODO: Check and Refine
  ProgressBoxDecorator::runProgressBox(
      this,
      [&]() -> bool {
        try {
          backup_->BackupDirectory();
          Logger::SystemLog("GUI | Backup created successfully.");
          return true;
        } catch (const std::exception& e) {
          Logger::SystemLog(
              "GUI | Cannot create backup: " + std::string(e.what()),
              LogLevel::ERROR);
          return false;
        }
      },
      "Creating backup...", "Backup created successfully.",
      "Backup creation failed.",
      [&](bool success) {
        if (success) {
          ui->stackedWidget->setCurrentIndex(0);
        } else {
          ui->nextButton->setEnabled(true);
        }
      });
}
