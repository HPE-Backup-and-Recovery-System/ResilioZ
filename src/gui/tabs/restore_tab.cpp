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
#include "gui/dialog/use_repository_dialog.h"
#include "gui/tabs/ui_restore_tab.h"
#include "utils/utils.h"

RestoreTab::RestoreTab(QWidget* parent)
    : QWidget(parent), ui(new Ui::RestoreTab) {
  ui->setupUi(this);

  ui->stackedWidget->setCurrentIndex(0);
  ui->stackedWidget_attemptRestore->setCurrentIndex(0);

  ui->backButton->setAutoDefault(true);
  ui->backButton->setDefault(false);
  ui->nextButton->setAutoDefault(true);
  ui->nextButton->setDefault(false);

  repository_ = nullptr;
  backup_file = "";
  backup_destination = "";

  connect(ui->stackedWidget_attemptRestore, &QStackedWidget::currentChanged,
          this, &RestoreTab::onAttemptRestorePageChanged);

  updateButtons();
  updateProgress();
}

RestoreTab::~RestoreTab() {
  delete ui;
  if (repository_ != nullptr) {
    delete repository_;
  }
}

void RestoreTab::updateProgress() {
  int index = ui->stackedWidget_attemptRestore->currentIndex();
  int total = ui->stackedWidget_attemptRestore->count();
  int percent = ((index + 1) * 100) / total;
  ui->progressBar->setValue(percent);
}

void RestoreTab::updateButtons() {
  int index = ui->stackedWidget_attemptRestore->currentIndex();
  int total = ui->stackedWidget_attemptRestore->count();
  if (index == 0) {
    ui->backButton->setStyleSheet("QPushButton { opacity: 0; }");

    if (repository_ != nullptr) {
      ui->nextButton->setEnabled(true);
    } else {
      ui->nextButton->setEnabled(false);
    }

  } else if (index == 1) {
    ui->backButton->setStyleSheet("QPushButton { opacity: 1; }");
    ui->nextButton->setEnabled(false);
  } else {
    ui->backButton->setStyleSheet("QPushButton { opacity: 1; }");
    ui->backButton->setEnabled(true);
  }

  if (index == total - 1) {
    ui->nextButton->setText("Start");
  } else {
    ui->nextButton->setText("Next");
  }
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

void RestoreTab::on_chooseRepoButton_clicked() {
  UseRepositoryDialog dialog(this);
  dialog.setWindowFlags(Qt::Window);
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

void RestoreTab::onAttemptRestorePageChanged(int index) {
  updateButtons();
  // Loading the backup file entries.
  if (index == 1) {
    QTimer::singleShot(0, this, [this]() { loadFileTable(); });
  }
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
}

void RestoreTab::loadFileTable() {
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

  // Polished Features for Table
  auto* header = ui->fileTable->horizontalHeader();
  ui->fileTable->setEditTriggers(QAbstractItemView::NoEditTriggers);
  ui->fileTable->setSelectionBehavior(QAbstractItemView::SelectRows);
  ui->fileTable->setSelectionMode(QAbstractItemView::SingleSelection);
  ui->fileTable->setShowGrid(true);
  ui->fileTable->setFocusPolicy(Qt::NoFocus);
  header->setStretchLastSection(false);
  header->setSectionResizeMode(0, QHeaderView::Stretch);
  ui->fileTable->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);
  ui->fileTable->verticalHeader()->setVisible(false);

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
    QTableWidgetItem* loading_failed =
        new QTableWidgetItem("< Failed to Load >");
    ui->fileTable->setItem(0, 0, loading_failed);
    loading_failed->setTextAlignment(Qt::AlignCenter);
    loading_failed->setForeground(Qt::darkGray);
    ui->fileTable->setSpan(0, 0, 1, 4);
    return;
  }

  ui->fileTable->setRowCount(backupList.size() + 1);
  ui->fileTable->setColumnCount(4);

  QStringList headers = {"Name", "Type", "Time", "Remarks"};

  for (int i = 0; i < headers.size(); ++i) {
    QTableWidgetItem* headerItem = new QTableWidgetItem(headers[i]);
    headerItem->setTextAlignment(Qt::AlignCenter);
    ui->fileTable->setHorizontalHeaderItem(i, headerItem);
  }

  if (backupList.size() == 0) {
    auto* noSelection = new QTableWidgetItem("< No Backups Found >");
    noSelection->setTextAlignment(Qt::AlignCenter);
    noSelection->setForeground(Qt::darkGray);
    ui->fileTable->setItem(0, 0, noSelection);
    ui->fileTable->setSpan(0, 0, 1, 4);
    return;
  }

  auto* noSelection = new QTableWidgetItem("< No Selection >");
  noSelection->setTextAlignment(Qt::AlignCenter);
  noSelection->setForeground(Qt::darkGray);
  ui->fileTable->setItem(0, 0, noSelection);
  ui->fileTable->setSpan(0, 0, 1, 4);

  for (size_t i = 0; i < backupList.size(); ++i) {
    BackupDetails backupInfo = backupList.at(i);

    QString name_ = QString::fromStdString(backupInfo.name);
    QString type_ = QString::fromStdString(backupInfo.type);
    QString timestamp_ = QString::fromStdString(backupInfo.timestamp);
    QString remarks_ = QString::fromStdString(backupInfo.remarks);

    QTableWidgetItem* nameItem = new QTableWidgetItem(name_);
    ui->fileTable->setItem(i + 1, 0, nameItem);
    nameItem->setToolTip(name_);
    nameItem->setTextAlignment(Qt::AlignCenter);

    QTableWidgetItem* typeItem = new QTableWidgetItem(type_);
    ui->fileTable->setItem(i + 1, 1, typeItem);
    typeItem->setToolTip(type_);
    typeItem->setTextAlignment(Qt::AlignCenter);

    QTableWidgetItem* timeItem = new QTableWidgetItem(timestamp_);
    ui->fileTable->setItem(i + 1, 2, timeItem);
    timeItem->setToolTip(timestamp_);
    timeItem->setTextAlignment(Qt::AlignCenter);

    QTableWidgetItem* remarksItem = new QTableWidgetItem(remarks_);
    ui->fileTable->setItem(i + 1, 3, remarksItem);
    remarksItem->setToolTip(remarks_);
    // remarksItem->setTextAlignment(Qt::AlignCenter);
  }

  ui->fileTable->selectRow(0);

  disconnect(ui->fileTable, &QTableWidget::itemSelectionChanged, this,
             &RestoreTab::onFileSelected);
  connect(ui->fileTable, &QTableWidget::itemSelectionChanged, this,
          &RestoreTab::onFileSelected);

  setColSize(ui->fileTable->viewport()->width());
}

void RestoreTab::onFileSelected() {
  QModelIndexList selected = ui->fileTable->selectionModel()->selectedRows();
  bool validSelection = !selected.isEmpty() && selected.first().row() != 0;

  if (validSelection) {
    backup_file =
        ui->fileTable->item(selected.first().row(), 0)->text().toStdString();
  }
  ui->nextButton->setEnabled(validSelection);
}

// Handle repo selection
bool RestoreTab::handleSelectRepo() {
  if (repository_ == nullptr) {
    MessageBoxDecorator::showMessageBox(
        this, "No Repository Selected",
        "Please select a repository to continue.", QMessageBox::Warning);
    ui->repoInfoLabel->setText("<NONE>");
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
    if (restore_) {
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
            ui->stackedWidget_attemptRestore->setCurrentIndex(0);
            updateProgress();
            updateButtons();
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

void RestoreTab::on_customDestination_clicked() {
  ui->destInput->setEnabled(true);
}

void RestoreTab::on_originalDestination_clicked() {
  ui->destInput->setText("");
  ui->destInput->setEnabled(false);
}
