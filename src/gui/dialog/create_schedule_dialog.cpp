#include "gui/dialog/create_schedule_dialog.h"

#include <QDir>

#include "backup_restore/backup.hpp"
#include "gui/decorators/message_box.h"
#include "gui/decorators/progress_box.h"
#include "gui/dialog/ui_create_schedule_dialog.h"
#include "gui/dialog/use_repository_dialog.h"
#include "utils/utils.h"

CreateScheduleDialog::CreateScheduleDialog(QWidget* parent)
    : QDialog(parent), ui(new Ui::CreateScheduleDialog) {
  ui->setupUi(this);

  ui->stackedWidget_createSchedule->setCurrentIndex(0);
  request_mgr = new SchedulerRequestManager();

  cron_string = "";
  source_path = "";
  repository_ = nullptr;
  remarks = "";
  type = BackupType::FULL;

  checkRepoSelection();
}

CreateScheduleDialog::~CreateScheduleDialog() {
  delete ui;
  delete request_mgr;
  delete repository_;
}

void CreateScheduleDialog::updateProgress() {
  int index = ui->stackedWidget_createSchedule->currentIndex();
  int total = ui->stackedWidget_createSchedule->count();
  int percent = ((index + 1) * 100) / total;
  ui->progressBar->setValue(percent);
}

void CreateScheduleDialog::updateButtons() {
  int index = ui->stackedWidget_createSchedule->currentIndex();
  int total = ui->stackedWidget_createSchedule->count();
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

void CreateScheduleDialog::checkRepoSelection() {
  ui->nextButton->setEnabled(repository_ != nullptr);
}

void CreateScheduleDialog::on_nextButton_clicked() {
  int index = ui->stackedWidget_createSchedule->currentIndex();
  int total = ui->stackedWidget_createSchedule->count();

  bool valid = true;
  switch (index) {
    case 0:
      valid = handleEndpointDetails();
      break;
    case 1:
      valid = handleBackupDetails();
      break;
    case 2:
      valid = handleScheduleDetails();
      break;
    default:
      break;
  }
  if (!valid) {
    return;
  }

  if (index < total - 1) {
    ui->stackedWidget_createSchedule->setCurrentIndex(index + 1);
  } else {
    ui->nextButton->setEnabled(false);
    addSchedule();
  }
  updateProgress();
}

void CreateScheduleDialog::on_backButton_clicked() {
  int index = ui->stackedWidget_createSchedule->currentIndex();
  if (index > 0) {
    ui->stackedWidget_createSchedule->setCurrentIndex(index - 1);
  } else {
    reject();
  }
  updateProgress();
}

bool CreateScheduleDialog::handleEndpointDetails() {
  std::string source_path_ = ui->sourceInput->text().toStdString();
  if (source_path_ == "") {
    source_path_ = ".";
  }

  if (!Validator::IsValidPath(source_path_)) {
    MessageBoxDecorator::showMessageBox(this, "Invalid Source",
                                        "Source path is invalid.",
                                        QMessageBox::Warning);
    return false;
  }

  if (repository_ == nullptr) {
    MessageBoxDecorator::showMessageBox(
        this, "Repository Missing", "Please select a repository to continue.",
        QMessageBox::Warning);
    return false;
  }

  std::string destination_path_ = repository_->GetFullPath();
  if (destination_path_ == "" || !Validator::IsValidPath(destination_path_)) {
    MessageBoxDecorator::showMessageBox(this, "Invalid Destination",
                                        "Destination is invalid.",
                                        QMessageBox::Warning);
    return false;
  }

  source_path = source_path_;
  return true;
}

bool CreateScheduleDialog::handleBackupDetails() {
  BackupType type_;
  if (ui->incButton->isChecked()) {
    type_ = BackupType::INCREMENTAL;
  } else if (ui->diffButton->isChecked()) {
    type_ = BackupType::DIFFERENTIAL;
  } else {
    type_ = BackupType::FULL;
  }

  std::string remarks_ = ui->remarksInput->text().toStdString();

  type = type_;
  remarks = remarks_;
  return true;
}

bool CreateScheduleDialog::handleScheduleDetails() {
  std::string cron_string_ = ui->scheduleInput->text().toStdString();

  if (cron_string_ == "" || !Validator::IsValidCronString(cron_string_)) {
    MessageBoxDecorator::showMessageBox(this, "Invalid CRON String",
                                        "CRON String is invalid.",
                                        QMessageBox::Warning);
    return false;
  }
  cron_string = cron_string_;
  return true;
}

void CreateScheduleDialog::addSchedule() {
  ProgressBoxDecorator::runProgressBoxIndeterminate(
      this,
      [&](auto setWaitMessage, auto setSuccessMessage,
          auto setFailureMessage) -> bool {
        try {
          setWaitMessage("Checking if source exists...");
          if (!QDir(QString::fromStdString(source_path)).exists()) {
            setFailureMessage("Source path could not be found.");
            return false;
          }

          setWaitMessage("Checking if destination exists...");
          if (!repository_->Exists()) {
            setFailureMessage("Destination path could not be found.");
            return false;
          }

          setWaitMessage("Creating schedule...");

          std::string new_schedule_id = request_mgr->SendAddRequest(
              cron_string, source_path, repository_->GetName(),
              repository_->GetPath(), repository_->GetPassword(), "",
              repository_->GetType(), remarks, type);

          bool validation = true;
          if (!validation) {
            setFailureMessage("Failed to add schedule.");
            return false;
          }

          Logger::SystemLog("Schedule " + new_schedule_id +
                            "created successfully.");
          setSuccessMessage("Schedule " +
                            QString::fromStdString(new_schedule_id) +
                            " created successfully.");
          return true;

        } catch (const std::exception& e) {
          std::string issue = e.what();
          Logger::SystemLog("Schedule creation failed: " + issue);
          setFailureMessage("Schedule creation failed: " +
                            QString::fromStdString(issue));
          return false;
        }
      },
      "Creating schedule...", "Schedule created successfully.",
      "Failed to create schedule.",
      [&](bool success) {
        if (success) {
          accept();  // Close dialog or notify success
        } else {
          ui->nextButton->setEnabled(true);
        }
      });
}
void CreateScheduleDialog::on_repoSelect_clicked() {
  UseRepositoryDialog dialog(this);
  if (dialog.exec() == QDialog::Accepted) {
    repository_ = dialog.getRepository();
    ui->repoInput->setText(QString::fromStdString(repository_->GetFullPath()));
    ui->repoInput->setToolTip(
        QString::fromStdString(repository_->GetRepositoryInfoString()));
  } else {
    repository_ = nullptr;
    ui->repoInput->setText("<NONE>");
  }
  checkRepoSelection();
}
