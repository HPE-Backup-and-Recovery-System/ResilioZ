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
#include "backup_restore/backup.hpp"
#include "backup_restore/restore.hpp"

RestoreTab::RestoreTab(QWidget* parent)
    : QWidget(parent), ui(new Ui::RestoreTab) {
  ui->setupUi(this);

  ui->stackedWidget->setCurrentIndex(0);
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
    ui->backButton->setEnabled(false);

    ui->nextButton->setEnabled(false);
    if (repository_){
      ui->nextButton->setEnabled(true);
    }
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
    QString repoMessage = QString::fromStdString(repository_->GetRepositoryInfoString());
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
  if (index == 1) {
    // Loading the backup file entries.
    QTimer::singleShot(0, this, [this]() { loadFileTable(); });
  }
}

void RestoreTab::resizeEvent(QResizeEvent* event) {
  QWidget::resizeEvent(event);
  setColSize(ui->fileTable->viewport()->width());
}

void RestoreTab::setColSize(int tableWidth){
  int col_name = 200;
  int col_type = 200;
  int col_time = 250;
  int col_rem = tableWidth - col_name - col_type - col_time;

  ui->fileTable->setColumnWidth(0, col_name);  // Name
  ui->fileTable->setColumnWidth(1, col_type);  // Type
  ui->fileTable->setColumnWidth(2, col_time);  // Time
  ui->fileTable->setColumnWidth(3, col_rem);   // Remarks
}

void RestoreTab::loadFileTable() {
  // Clear previous data
  ui->fileTable->clearContents();

  auto* header = ui->fileTable->horizontalHeader();
  

  // Polished Features for Table
  ui->fileTable->setEditTriggers(QAbstractItemView::NoEditTriggers);
  ui->fileTable->setSelectionBehavior(QAbstractItemView::SelectRows);
  ui->fileTable->setSelectionMode(QAbstractItemView::SingleSelection);
  ui->fileTable->setShowGrid(true);
  ui->fileTable->setFocusPolicy(Qt::NoFocus);
  header->setStretchLastSection(false);
  header->setSectionResizeMode(0, QHeaderView::Stretch);
  ui->fileTable->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);
  ui->fileTable->verticalHeader()->setVisible(false);

  if (!repository_){
    return;
  }

  Backup backup(repository_, ".");
  std::vector<BackupDetails> backupList = backup.GetAllBackupDetails();

  ui->fileTable->setRowCount(backupList.size() + 1);
  ui->fileTable->setColumnCount(4);

  auto* noSelection = new QTableWidgetItem("< No Selection >");
  noSelection->setTextAlignment(Qt::AlignCenter);
  noSelection->setForeground(Qt::darkGray);
  ui->fileTable->setItem(0, 0, noSelection);
  ui->fileTable->setSpan(0,0,1,4);

  QStringList headers = {"File Name", "Backup Type", "Timestamp", "Remarks"};

  for (int i = 0; i < headers.size(); ++i) {
      QTableWidgetItem* headerItem = new QTableWidgetItem(headers[i]);
      headerItem->setTextAlignment(Qt::AlignCenter);
      ui->fileTable->setHorizontalHeaderItem(i, headerItem);
  }


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
    remarksItem->setTextAlignment(Qt::AlignCenter);
  }
  
  connect(ui->fileTable, &QTableWidget::itemSelectionChanged, this,
          &RestoreTab::onFileSelected);

  setColSize(ui->fileTable->viewport()->width());
}

void RestoreTab::onFileSelected() {
  QList<QTableWidgetItem*> selectedItems = ui->fileTable->selectedItems();
  if (!selectedItems.isEmpty()) {
    backup_file = selectedItems.first()->text().toStdString();
  } else {
    backup_file = "";
  }
}


// Handle repo selection
bool RestoreTab::handleSelectRepo(){
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
  backup_destination = "/";
  if (ui->customDestination->isChecked()){
    backup_destination = ui->destInput->text().toStdString();
  }

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
          Restore restore(repository_);
          restore.RestoreAll(backup_destination,backup_file);

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

void RestoreTab::on_customDestination_clicked()
{
  ui->destInput->setEnabled(true);
}

void RestoreTab::on_originalDestination_clicked()
{
  ui->destInput->setText("");
  ui->destInput->setEnabled(false);
}

