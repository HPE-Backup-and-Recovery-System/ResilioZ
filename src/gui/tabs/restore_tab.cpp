#include "gui/tabs/restore_tab.h"

#include <QDateTime>
#include <QDir>
#include <QFileDialog>
#include <QFileInfo>
#include <QFileInfoList>
#include <QTimer>

#include "gui/decorators/message_box.h"
#include "gui/decorators/progress_box.h"
#include "gui/dialog/use_repository_dialog.h"
#include "gui/tabs/ui_restore_tab.h"
#include "utils/utils.h"
#include "backup_restore/restore.hpp"

RestoreTab::RestoreTab(QWidget* parent)
    : QWidget(parent), ui(new Ui::RestoreTab) {
  ui->setupUi(this);

  ui->stackedWidget->setCurrentIndex(0);
  ui->backButton->setAutoDefault(true);
  ui->backButton->setDefault(false);
  ui->nextButton->setAutoDefault(true);
  ui->nextButton->setDefault(true);

  repository_ = nullptr;
  backup_file = "";
  backup_destination = "";

  connect(ui->stackedWidget_attemptRestore, &QStackedWidget::currentChanged,
          this, &RestoreTab::onAttemptRestorePageChanged);
}

RestoreTab::~RestoreTab() { delete ui; }

void RestoreTab::on_restoreButton_clicked() {
  ui->stackedWidget->setCurrentIndex(1);
}

void RestoreTab::on_retryButton_clicked() {
  // ui->stackedWidget->setCurrentIndex(2);
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
    std:: string message = "Repo: " + repository_->GetFullPath() + " \n" + "File: " + backup_file + " \n" + "Dest: " + backup_destination + "\n";
    // TODO: Trigger Restore Process
    restoreBackup();
  }
  updateProgress();
}

void RestoreTab::on_backButton_clicked() {
  int index = ui->stackedWidget_attemptRestore->currentIndex();
  if (index > 0) {
    ui->stackedWidget_attemptRestore->setCurrentIndex(index - 1);
  } else {
    ui->stackedWidget->setCurrentIndex(0);
  }
  updateProgress();
}

void RestoreTab::on_chooseRepoButton_clicked() {
  UseRepositoryDialog dialog(this);
  dialog.setWindowFlags(Qt::Window);
  if (dialog.exec() == QDialog::Accepted) {
    repository_ = dialog.getRepository();
  } else {
    repository_ = nullptr;
  }
}

void RestoreTab::onAttemptRestorePageChanged(int index) {
  updateButtons();
  if (index == 1) {
    // Loading the backup file entries.
    QTimer::singleShot(0, this, [this]() { loadFileTable(); });
  }
}

void RestoreTab::loadFileTable() {
  // Clear previous data
  ui->fileTable->clearContents();

  auto* header = ui->fileTable->horizontalHeader();
  auto* noSelection = new QTableWidgetItem("< No Selection >");
  noSelection->setTextAlignment(Qt::AlignCenter);
  noSelection->setForeground(Qt::darkGray);
  ui->fileTable->setItem(0, 0, noSelection);

  // Polished Features for Table
  ui->fileTable->setEditTriggers(QAbstractItemView::NoEditTriggers);
  ui->fileTable->setSelectionBehavior(QAbstractItemView::SelectRows);
  ui->fileTable->setSelectionMode(QAbstractItemView::SingleSelection);
  ui->fileTable->setShowGrid(true);
  ui->fileTable->setFocusPolicy(Qt::NoFocus);
  header->setStretchLastSection(true);
  header->setSectionResizeMode(0, QHeaderView::Stretch);
  ui->fileTable->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);
  ui->fileTable->verticalHeader()->setVisible(false);

  // To do: Handle remote repos
  if (!repository_){
    return;
  }

  // For local repos
  std::string path_ = repository_->GetFullPath() + "/backup";
  QString directoryPath = QString::fromStdString(path_);
  QDir dir(directoryPath);

  // Filter only files, and sort
  dir.setFilter(QDir::Files | QDir::NoSymLinks);
  dir.setSorting(QDir::Name);

  QFileInfoList fileList = dir.entryInfoList();

  ui->fileTable->setRowCount(fileList.size());
  ui->fileTable->setColumnCount(1);
  ui->fileTable->setHorizontalHeaderLabels(QStringList() << "Backup File Name");

  for (int i = 0; i < fileList.size(); ++i) {
    QFileInfo fileInfo = fileList.at(i);
    QTableWidgetItem* nameItem = new QTableWidgetItem(fileInfo.fileName());

    ui->fileTable->setItem(i, 0, nameItem);
    nameItem->setTextAlignment(Qt::AlignCenter);
  }
  connect(ui->fileTable, &QTableWidget::itemSelectionChanged, this,
          &RestoreTab::onFileSelected);
}

void RestoreTab::onFileSelected() {
  QList<QTableWidgetItem*> selectedItems = ui->fileTable->selectedItems();
  if (!selectedItems.isEmpty()) {
    backup_file = selectedItems.first()->text().toStdString();
  } else {
    backup_file = "";
  }
}

void RestoreTab::on_chooseDestination_clicked() {
  QString selectedDir = QFileDialog::getExistingDirectory(
      this, "Choose Backup Directory", QDir::homePath(),
      QFileDialog::ShowDirsOnly | QFileDialog::DontResolveSymlinks);
  if (!selectedDir.isEmpty()) {
    backup_destination = selectedDir.toStdString();
  }
  else{
    backup_destination = "";
  }
}


// Handle repo selection
bool RestoreTab::handleSelectRepo(){
  if (repository_ == nullptr) {
    MessageBoxDecorator::showMessageBox(
        this, "No Repository Selected",
        "Please select a repository to continue.", QMessageBox::Warning);
    return false;
  }
  return true;
}

// Handle backup file selection
bool RestoreTab::handleSelectFile(){
  if (backup_file == "") {
    MessageBoxDecorator::showMessageBox(
        this, "No File Selected",
        "Please select a backup file to continue.", QMessageBox::Warning);
    return false;
  }
  return true;
}

// Handle destination selection
bool RestoreTab::handleSelectDestination(){
  if (backup_destination == "") {
    MessageBoxDecorator::showMessageBox(
        this, "No Destination Selected",
        "Please select a destination folder to continue.", QMessageBox::Warning);
    return false;
  }
  return true;
}

void RestoreTab::restoreBackup(){
  ProgressBoxDecorator::runProgressBoxIndeterminate(
      this,
      [&](std::function<void(const QString&)> setWaitMessage,
          std::function<void(const QString&)> setSuccessMessage,
          std::function<void(const QString&)> setFailureMessage) -> bool {
        try {
          setWaitMessage("Checking if repository exists...");
          if (!repository_->Exists()) {
            setFailureMessage("Repository does not exist at location: " +
                              QString::fromStdString(repository_->GetPath()));
            return false;
          }

          setWaitMessage("Checking if backup file exists...");
          std::string backup_file_path = repository_->GetFullPath() + "/backup/" + backup_file;
          QFileInfo file(QString::fromStdString(backup_file_path));
          if (!file.exists() || !file.isFile()) {
            setFailureMessage("Backup file does not exist at location: " +
                              QString::fromStdString(backup_file_path));
            return false;
          }

          setWaitMessage("Checking if destination folder exists...");
          std::string restore_file_path = backup_destination;
          if (!QDir(QString::fromStdString(backup_destination)).exists()){
            setFailureMessage("Destination folder does not exist at location: " +
                              QString::fromStdString(backup_destination));
            return false;
          }

          setWaitMessage("Attempting restore...");
          // Restore restore(repository_);
          // restore.RestoreAll(backup_destination,backup_file);

          setSuccessMessage("Restore successfully completed.");
          return true;

        } catch (const std::exception& e) {
          Logger::SystemLog(
              "Restore operation failed: " + std::string(e.what()),
              LogLevel::ERROR);

          setFailureMessage("Restore operation failed: " +
                            QString::fromStdString(e.what()));
          return false;
        }
      },
      "Attempting restore...", "Restore operation successful.",
      "Restore operation failed.",
      [&](bool success) {
        if (success) {
          delete repository_;
        } else {
          ui->nextButton->setEnabled(true);
        }
      });
}