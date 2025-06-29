#include "gui/tabs/restore_tab.h"

#include <QDateTime>
#include <QDir>
#include <QFileDialog>
#include <QFileInfo>
#include <QFileInfoList>
#include <QTimer>

#include "backup_restore/backup.hpp"
#include "backup_restore/restore.hpp"
#include "gui/decorators/message_box.h"
#include "gui/decorators/progress_box.h"
#include "gui/dialog/backup_verification_dialog.h"
#include "gui/dialog/use_repository_dialog.h"
#include "gui/tabs/ui_restore_tab.h"
#include "utils/utils.h"

RestoreTab::RestoreTab(QWidget* parent)
    : QWidget(parent), ui(new Ui::RestoreTab) {
  ui->setupUi(this);

  ui->stackedWidget->setCurrentIndex(0);
  ui->stackedWidget_attemptRestore->setCurrentIndex(0);
  ui->stackedWidget_verifyBackup->setCurrentIndex(0);

  ui->backButton->setAutoDefault(true);
  ui->backButton->setDefault(false);
  ui->nextButton->setAutoDefault(true);
  ui->nextButton->setDefault(false);

  ui->backButton_2->setAutoDefault(true);
  ui->backButton_2->setDefault(false);
  ui->nextButton_2->setAutoDefault(true);
  ui->nextButton_2->setDefault(false);

  ui->backButton_3->setAutoDefault(true);
  ui->backButton_3->setDefault(false);
  ui->nextButton_3->setAutoDefault(true);
  ui->nextButton_3->setDefault(false);

  repository_ = nullptr;
  restore_ = nullptr;
  backup_file = "";
  backup_destination = "";

  connect(ui->stackedWidget_attemptRestore, &QStackedWidget::currentChanged,
          this, &RestoreTab::onAttemptRestorePageChanged);

  connect(ui->stackedWidget_verifyBackup, &QStackedWidget::currentChanged, this,
          &RestoreTab::onVerifyBackupPageChanged);

  updateButtons();
  updateProgress();
}

RestoreTab::~RestoreTab() {
  delete ui;
  if (restore_ != nullptr) delete restore_;
  if (repository_ != nullptr) delete repository_;
}

void RestoreTab::resizeEvent(QResizeEvent* event) {
  QWidget::resizeEvent(event);
  setColSize(ui->fileTable->viewport()->width());
}

void RestoreTab::setColSize(int tableWidth) {
  int col_name = 300;
  int col_type = 200;
  int col_time = 300;
  int col_rem = tableWidth - col_name - col_type - col_time;

  ui->fileTable->setColumnWidth(0, col_name);  // Name
  ui->fileTable->setColumnWidth(1, col_type);  // Type
  ui->fileTable->setColumnWidth(2, col_time);  // Time
  ui->fileTable->setColumnWidth(3, col_rem);   // Remarks

  ui->fileTable_2->setColumnWidth(0, col_name);  // Name
  ui->fileTable_2->setColumnWidth(1, col_type);  // Type
  ui->fileTable_2->setColumnWidth(2, col_time);  // Time
  ui->fileTable_2->setColumnWidth(3, col_rem);   // Remarks
}

void RestoreTab::updateProgress() {
  switch (ui->stackedWidget->currentIndex()) {
    case 1: {
      ui->progressBar->setValue(
          ((ui->stackedWidget_attemptRestore->currentIndex() + 1) * 100) /
          ui->stackedWidget_attemptRestore->count());
      break;
    }
    case 2: {
      ui->progressBar_2->setValue(
          ((ui->stackedWidget_verifyBackup->currentIndex() + 1) * 100) /
          ui->stackedWidget_verifyBackup->count());
      break;
    }
    default:
      break;
  }
}

void RestoreTab::updateButtons() {
  switch (ui->stackedWidget->currentIndex()) {
    case 1: {
      int index = ui->stackedWidget_attemptRestore->currentIndex();
      int total = ui->stackedWidget_attemptRestore->count();
      if (index == 0) {
        if (repository_ != nullptr) {
          ui->nextButton->setEnabled(true);
        } else {
          ui->nextButton->setEnabled(false);
        }

      } else if (index == 1) {
        ui->nextButton->setEnabled(false);
      }

      if (index == total - 1) {
        ui->nextButton->setText("Start");
      } else {
        ui->nextButton->setText("Next");
      }
      break;
    }
    case 2: {
      int index = ui->stackedWidget_verifyBackup->currentIndex();
      int total = ui->stackedWidget_verifyBackup->count();
      if (index == 0) {
        if (repository_ != nullptr) {
          ui->nextButton_2->setEnabled(true);
        } else {
          ui->nextButton_2->setEnabled(false);
        }

      } else if (index == 1) {
        ui->nextButton_2->setEnabled(false);
      }

      if (index == total - 1) {
        ui->nextButton_2->setText("Start");
      } else {
        ui->nextButton_2->setText("Next");
      }
      break;
    }
    default:
      break;
  }
}

void RestoreTab::loadFileTables() {
  if (!repository_) {
    MessageBoxDecorator::showMessageBox(
        this, "Repository missing",
        "Repository not selected for backup file fetch", QMessageBox::Critical);
    Logger::SystemLog("GUI | Restore | Repository not selected: ",
                      LogLevel::ERROR);
    return;
  }

  // Clear previous data
  ui->fileTable->clearContents();
  ui->fileTable_2->clearContents();

  // Polished Features for Tables
  auto* header_restore = ui->fileTable->horizontalHeader();
  ui->fileTable->setEditTriggers(QAbstractItemView::NoEditTriggers);
  ui->fileTable->setSelectionBehavior(QAbstractItemView::SelectRows);
  ui->fileTable->setSelectionMode(QAbstractItemView::SingleSelection);
  ui->fileTable->setShowGrid(true);
  ui->fileTable->setFocusPolicy(Qt::NoFocus);
  header_restore->setStretchLastSection(false);
  header_restore->setSectionResizeMode(0, QHeaderView::Stretch);
  ui->fileTable->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);
  ui->fileTable->verticalHeader()->setVisible(false);

  auto* header_verify = ui->fileTable_2->horizontalHeader();
  ui->fileTable_2->setEditTriggers(QAbstractItemView::NoEditTriggers);
  ui->fileTable_2->setSelectionBehavior(QAbstractItemView::SelectRows);
  ui->fileTable_2->setSelectionMode(QAbstractItemView::SingleSelection);
  ui->fileTable_2->setShowGrid(true);
  ui->fileTable_2->setFocusPolicy(Qt::NoFocus);
  header_verify->setStretchLastSection(false);
  header_verify->setSectionResizeMode(0, QHeaderView::Stretch);
  ui->fileTable_2->setSizePolicy(QSizePolicy::Expanding,
                                 QSizePolicy::Expanding);
  ui->fileTable_2->verticalHeader()->setVisible(false);

  std::vector<BackupDetails> backupList;
  try {
    Backup backup(repository_, ".");
    backupList = backup.GetAllBackupDetails();
  }

  catch (const std::exception& e) {
    MessageBoxDecorator::showMessageBox(this, "Backup files fetch error",
                                        QString::fromStdString(e.what()),
                                        QMessageBox::Critical);
    Logger::SystemLog("GUI | Restore | Failed to fetch backup files: " +
                          std::string(e.what()),
                      LogLevel::ERROR);

    ui->fileTable->setRowCount(1);
    ui->fileTable_2->setRowCount(1);
    QTableWidgetItem *loading_failed_restore =
                         new QTableWidgetItem("< Failed to Load >"),
                     *loading_failed_verify =
                         new QTableWidgetItem("< Failed to Load >");

    ui->fileTable->setItem(0, 0, loading_failed_restore);
    ui->fileTable_2->setItem(0, 0, loading_failed_verify);

    loading_failed_restore->setTextAlignment(Qt::AlignCenter);
    loading_failed_restore->setForeground(Qt::darkGray);

    loading_failed_verify->setTextAlignment(Qt::AlignCenter);
    loading_failed_verify->setForeground(Qt::darkGray);

    ui->fileTable->setSpan(0, 0, 1, 4);
    ui->fileTable_2->setSpan(0, 0, 1, 4);
    return;
  }

  int backup_count = static_cast<int>(backupList.size());

  ui->fileTable->setRowCount(backup_count + 1);
  ui->fileTable_2->setRowCount(backup_count + 1);
  ui->fileTable->setColumnCount(4);
  ui->fileTable_2->setColumnCount(4);

  QStringList headers = {"Name", "Type", "Time", "Remarks"};

  for (int i = 0; i < headers.size(); ++i) {
    QTableWidgetItem* headerItem_restore = new QTableWidgetItem(headers[i]);
    headerItem_restore->setTextAlignment(Qt::AlignCenter);
    ui->fileTable->setHorizontalHeaderItem(i, headerItem_restore);

    QTableWidgetItem* headerItem_verify = new QTableWidgetItem(headers[i]);
    headerItem_verify->setTextAlignment(Qt::AlignCenter);
    ui->fileTable_2->setHorizontalHeaderItem(i, headerItem_verify);
  }

  if (backup_count == 0) {
    auto* noSelection_restore = new QTableWidgetItem("< No Backups Found >");
    noSelection_restore->setTextAlignment(Qt::AlignCenter);
    noSelection_restore->setForeground(Qt::darkGray);
    ui->fileTable->setItem(0, 0, noSelection_restore);

    auto* noSelection_verify = new QTableWidgetItem("< No Backups Found >");
    noSelection_verify->setTextAlignment(Qt::AlignCenter);
    noSelection_verify->setForeground(Qt::darkGray);
    ui->fileTable_2->setItem(0, 0, noSelection_verify);

    ui->fileTable->setSpan(0, 0, 1, 4);
    ui->fileTable_2->setSpan(0, 0, 1, 4);
    return;
  }

  auto* noSelection_restore = new QTableWidgetItem("< No Selection >");
  noSelection_restore->setTextAlignment(Qt::AlignCenter);
  noSelection_restore->setForeground(Qt::darkGray);
  ui->fileTable->setItem(0, 0, noSelection_restore);
  ui->fileTable->setSpan(0, 0, 1, 4);

  auto* noSelection_verify = new QTableWidgetItem("< No Selection >");
  noSelection_verify->setTextAlignment(Qt::AlignCenter);
  noSelection_verify->setForeground(Qt::darkGray);
  ui->fileTable_2->setItem(0, 0, noSelection_verify);
  ui->fileTable_2->setSpan(0, 0, 1, 4);

  for (size_t i = 0; i < backup_count; ++i) {
    BackupDetails backupInfo = backupList.at(i);

    QString name_ = QString::fromStdString(backupInfo.name);
    QString type_ = QString::fromStdString(backupInfo.type);
    QString timestamp_ = QString::fromStdString(backupInfo.timestamp);
    QString remarks_ = QString::fromStdString(backupInfo.remarks);

    QTableWidgetItem *nameItem_restore = new QTableWidgetItem(name_),
                     *nameItem_verify = new QTableWidgetItem(name_);
    ui->fileTable->setItem(i + 1, 0, nameItem_restore);
    ui->fileTable_2->setItem(i + 1, 0, nameItem_verify);
    nameItem_restore->setTextAlignment(Qt::AlignCenter);
    nameItem_verify->setTextAlignment(Qt::AlignCenter);

    QTableWidgetItem *typeItem_restore = new QTableWidgetItem(type_),
                     *typeItem_verify = new QTableWidgetItem(type_);
    ui->fileTable->setItem(i + 1, 1, typeItem_restore);
    ui->fileTable_2->setItem(i + 1, 1, typeItem_verify);
    typeItem_restore->setTextAlignment(Qt::AlignCenter);
    typeItem_verify->setTextAlignment(Qt::AlignCenter);

    QTableWidgetItem *timeItem_restore = new QTableWidgetItem(timestamp_),
                     *timeItem_verify = new QTableWidgetItem(timestamp_);
    ui->fileTable->setItem(i + 1, 2, timeItem_restore);
    ui->fileTable_2->setItem(i + 1, 2, timeItem_verify);
    timeItem_restore->setTextAlignment(Qt::AlignCenter);
    timeItem_verify->setTextAlignment(Qt::AlignCenter);

    QTableWidgetItem *remarksItem_restore = new QTableWidgetItem(remarks_),
                     *remarksItem_verify = new QTableWidgetItem(remarks_);
    ui->fileTable->setItem(i + 1, 3, remarksItem_restore);
    ui->fileTable_2->setItem(i + 1, 3, remarksItem_verify);
    remarksItem_restore->setToolTip(remarks_);
    remarksItem_verify->setToolTip(remarks_);
    // remarksItem->setTextAlignment(Qt::AlignCenter);
  }

  ui->fileTable->selectRow(0);
  ui->fileTable_2->selectRow(0);

  disconnect(ui->fileTable, &QTableWidget::itemSelectionChanged, this,
             &RestoreTab::onFileSelected);
  disconnect(ui->fileTable_2, &QTableWidget::itemSelectionChanged, this,
             &RestoreTab::onFileSelected);

  connect(ui->fileTable, &QTableWidget::itemSelectionChanged, this,
          &RestoreTab::onFileSelected);
  connect(ui->fileTable_2, &QTableWidget::itemSelectionChanged, this,
          &RestoreTab::onFileSelected);

  setColSize(ui->fileTable->viewport()->width());
}

void RestoreTab::on_restoreButton_clicked() {
  prev_index_ = 1;
  repository_ = nullptr;
  ui->repoInfoLabel->setText("<NONE>");
  ui->nextButton->setEnabled(false);
  ui->stackedWidget->setCurrentIndex(1);
  ui->stackedWidget_attemptRestore->setCurrentIndex(0);
  updateProgress();
}

void RestoreTab::on_verifyButton_clicked() {
  prev_index_ = 2;
  repository_ = nullptr;
  ui->repoInfoLabel_2->setText("<NONE>");
  ui->nextButton_2->setEnabled(false);
  ui->stackedWidget->setCurrentIndex(2);
  ui->stackedWidget_verifyBackup->setCurrentIndex(0);
  updateProgress();
}

void RestoreTab::onFileSelected() {
  int index = ui->stackedWidget->currentIndex();
  QModelIndexList selected;
  switch (index) {
    case 1: {
      selected = ui->fileTable->selectionModel()->selectedRows();
      bool validSelection = !selected.isEmpty() && selected.first().row() != 0;

      if (validSelection) {
        backup_file = ui->fileTable->item(selected.first().row(), 0)
                          ->text()
                          .toStdString();
      }
      ui->nextButton->setEnabled(validSelection);
      break;
    }
    case 2: {
      selected = ui->fileTable_2->selectionModel()->selectedRows();
      bool validSelection = !selected.isEmpty() && selected.first().row() != 0;

      if (validSelection) {
        backup_file = ui->fileTable_2->item(selected.first().row(), 0)
                          ->text()
                          .toStdString();
      }
      ui->nextButton_2->setEnabled(validSelection);
      break;
    }
    default:
      break;
  }
}

void RestoreTab::on_chooseRepoButton_clicked() {
  if (repository_ != nullptr) {
    delete repository_;
    repository_ = nullptr;
  }

  UseRepositoryDialog dialog(this);
  if (dialog.exec() == QDialog::Accepted) {
    repository_ = dialog.getRepository();
    QString repoMessage =
        QString::fromStdString(repository_->GetRepositoryInfoString());
    ui->repoInfoLabel->setText(repoMessage);
    ui->nextButton->setEnabled(true);
  } else {
    repository_ = nullptr;
    ui->repoInfoLabel->setText("<NONE>");
    ui->nextButton->setEnabled(false);
  }
}

void RestoreTab::on_chooseRepoButton_2_clicked() {
  if (repository_ != nullptr) {
    delete repository_;
    repository_ = nullptr;
  }

  UseRepositoryDialog dialog(this);
  if (dialog.exec() == QDialog::Accepted) {
    repository_ = dialog.getRepository();
    QString repoMessage =
        QString::fromStdString(repository_->GetRepositoryInfoString());
    ui->repoInfoLabel_2->setText(repoMessage);
    ui->nextButton_2->setEnabled(true);
  } else {
    repository_ = nullptr;
    ui->repoInfoLabel_2->setText("<NONE>");
    ui->nextButton_2->setEnabled(false);
  }
}

void RestoreTab::on_customDestination_clicked() {
  ui->destInput->setEnabled(true);
}

void RestoreTab::on_originalDestination_clicked() {
  ui->destInput->setText("");
  ui->destInput->setEnabled(false);
}

void RestoreTab::onAttemptRestorePageChanged(int index) {
  updateButtons();
  // Loading the backup file entries.
  if (index == 1) {
    QTimer::singleShot(0, this, [this]() { loadFileTables(); });
  }
}

void RestoreTab::onVerifyBackupPageChanged(int index) {
  updateButtons();
  // Loading the backup file entries.
  if (index == 1) {
    QTimer::singleShot(0, this, [this]() { loadFileTables(); });
  }
}

void RestoreTab::on_backButton_clicked() {
  int index = ui->stackedWidget_attemptRestore->currentIndex();
  if (index > 0) {
    ui->stackedWidget_attemptRestore->setCurrentIndex(index - 1);
  } else {
    ui->stackedWidget->setCurrentIndex(0);
  }
  updateProgress();
  updateButtons();
}

void RestoreTab::on_backButton_2_clicked() {
  int index = ui->stackedWidget_verifyBackup->currentIndex();
  if (index > 0) {
    ui->stackedWidget_verifyBackup->setCurrentIndex(index - 1);
  } else {
    ui->stackedWidget->setCurrentIndex(0);
  }
  updateProgress();
  updateButtons();
}

void RestoreTab::on_backButton_3_clicked() {
  ui->stackedWidget->setCurrentIndex(prev_index_);
}

void RestoreTab::on_nextButton_clicked() {
  int index = ui->stackedWidget_attemptRestore->currentIndex();
  int total = ui->stackedWidget_attemptRestore->count();

  bool valid = true;
  switch (index) {
    case 0:
      valid = handleSelectRepo();
      break;
    case 1:
      valid = handleSelectFile();
      break;
    case 2:
      valid = handleSelectDestination();
      break;
    default:
      break;
  }

  if (!valid) {
    return;
  }

  if (index < total - 1) {
    ui->stackedWidget_attemptRestore->setCurrentIndex(index + 1);
  } else {
    restoreBackup();
  }
  updateProgress();
  updateButtons();
}

void RestoreTab::on_nextButton_2_clicked() {
  int index = ui->stackedWidget_verifyBackup->currentIndex();
  int total = ui->stackedWidget_verifyBackup->count();

  bool valid = true;
  switch (index) {
    case 0:
      valid = handleSelectRepo();
      break;
    case 1:
      valid = handleSelectFile();
      break;
    default:
      break;
  }

  if (!valid) {
    return;
  }

  if (index < total - 1) {
    ui->stackedWidget_verifyBackup->setCurrentIndex(index + 1);
  } else {
    verifyBackup();
  }
  updateProgress();
  updateButtons();
}

void RestoreTab::on_nextButton_3_clicked() {
  std::string window_title;
  std::string title_label;
  if (prev_index_ == 1) {
    window_title = "Restore Attempt Results";
    title_label = "Restore Attempt Results";
  } else {
    window_title = "Backup Verification Results";
    title_label = "Backup Verification Results";
  }

  BackupVerificationDialog dialog(
      this, restore_->GetSuccessFiles(), restore_->GetCorruptFiles(),
      restore_->GetFailedFiles(), window_title, title_label);
  dialog.exec();
}

bool RestoreTab::handleSelectRepo() {
  if (repository_ == nullptr) {
    MessageBoxDecorator::showMessageBox(
        this, "No Repository Selected",
        "Please select a repository to continue.", QMessageBox::Warning);
    ui->repoInfoLabel->setText("<NONE>");
    ui->repoInfoLabel_2->setText("<NONE>");
    return false;
  }
  return true;
}

// Handle backup file selection
bool RestoreTab::handleSelectFile() {
  if (QString::fromStdString(backup_file).trimmed().isEmpty()) {
    MessageBoxDecorator::showMessageBox(
        this, "No File Selected", "Please select a backup file to continue.",
        QMessageBox::Warning);
    return false;
  }
  return true;
}

// Handle destination selection
bool RestoreTab::handleSelectDestination() {
  backup_destination = "/";
  if (ui->customDestination->isChecked()) {
    backup_destination = ui->destInput->text().toStdString();
  }

  if (QString::fromStdString(backup_destination).trimmed().isEmpty() ||
      !Validator::IsValidPath(backup_destination)) {
    MessageBoxDecorator::showMessageBox(
        this, "Invalid Destination Selected",
        "Please enter a valid destination folder path to continue.",
        QMessageBox::Warning);
    return false;
  }

  // Check if destination exists and is a folder
  QFileInfo destInfo(QString::fromStdString(backup_destination));
  if (!destInfo.exists() || !destInfo.isDir()) {
    MessageBoxDecorator::showMessageBox(
        this, "Invalid Destination Selected",
        "Please enter an existing destination folder path to continue.",
        QMessageBox::Warning);
    return false;
  }

  return true;
}

void RestoreTab::restoreBackup() {
  try {
    if (restore_ != nullptr) {
      delete restore_;
      restore_ = nullptr;
    }

    restore_ = new RestoreGUI(this, repository_);
  } catch (const std::exception& e) {
    MessageBoxDecorator::showMessageBox(this, "Restore Initialization Failure",
                                        QString::fromStdString(e.what()),
                                        QMessageBox::Critical);
    Logger::SystemLog(
        "GUI | Failed to initialize restore: " + std::string(e.what()),
        LogLevel::ERROR);

    ui->nextButton->setEnabled(true);
    return;
  }

  try {
    restore_->RestoreAll(
        [this](bool success) {
          if (success) {
            ui->stackedWidget->setCurrentIndex(3);
            summary_ = restore_->GetRestoreSummary();
            ui->totalFiles->setText(QString::number(summary_.total_files));
            ui->processedFiles->setText(
                QString::number(summary_.processed_files));
            ui->successFiles->setText(QString::number(summary_.success_files));
            ui->failFiles->setText(QString::number(summary_.failed_files));
            ui->corruptFiles->setText(QString::number(summary_.corrupt_files));
            ui->statusField->setText(QString::fromStdString(summary_.status));

            ui->successLabel->setText("Successfully Restored Files");
            ui->failLabel->setText("Failed to Restore Files");
            ui->corruptLabel->setText("Failed Integrity Checks");

          } else {
            ui->nextButton->setEnabled(true);
          }
        },
        backup_destination, backup_file);
  } catch (const std::exception& e) {
    MessageBoxDecorator::showMessageBox(this, "Backup Finalization Failure",
                                        QString::fromStdString(e.what()),
                                        QMessageBox::Critical);
    Logger::SystemLog(
        "GUI | Failed to finalize restore: " + std::string(e.what()),
        LogLevel::ERROR);

    ui->nextButton->setEnabled(true);
  }
}

void RestoreTab::verifyBackup() {
  try {
    if (restore_ != nullptr) {
      delete restore_;
      restore_ = nullptr;
    }

    restore_ = new RestoreGUI(this, repository_);
  } catch (const std::exception& e) {
    MessageBoxDecorator::showMessageBox(
        this, "Verify Backup Initialization Failure",
        QString::fromStdString(e.what()), QMessageBox::Critical);
    Logger::SystemLog(
        "GUI | Failed to initialize verify backup: " + std::string(e.what()),
        LogLevel::ERROR);

    ui->nextButton_2->setEnabled(true);
    return;
  }

  try {
    restore_->VerifyBackup(
        [this](bool success) {
          if (success) {
            ui->stackedWidget->setCurrentIndex(3);
            summary_ = restore_->GetRestoreSummary();
            ui->totalFiles->setText(QString::number(summary_.total_files));
            ui->processedFiles->setText(
                QString::number(summary_.processed_files));
            ui->successFiles->setText(QString::number(summary_.success_files));
            ui->failFiles->setText(QString::number(summary_.failed_files));
            ui->corruptFiles->setText(QString::number(summary_.corrupt_files));
            ui->statusField->setText(QString::fromStdString(summary_.status));

            ui->successLabel->setText("Successfully Verified Files");
            ui->failLabel->setText("Failed to Verify Files");
            ui->corruptLabel->setText("Failed Integrity Checks");

          } else {
            ui->nextButton_2->setEnabled(true);
          }
        },
        backup_file);
  } catch (const std::exception& e) {
    MessageBoxDecorator::showMessageBox(
        this, "Verify Backup Finalization Failure",
        QString::fromStdString(e.what()), QMessageBox::Critical);
    Logger::SystemLog(
        "GUI | Failed to finalize verify backup: " + std::string(e.what()),
        LogLevel::ERROR);

    ui->nextButton_2->setEnabled(true);
  }
}