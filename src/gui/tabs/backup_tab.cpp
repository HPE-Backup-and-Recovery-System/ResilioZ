#include "gui/tabs/backup_tab.h"

#include <QModelIndexList>
#include <QThread>
#include <QTimer>

#include "gui/decorators/core.h"
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
  ui->stackedWidget_listBackup->setCurrentIndex(0);
  ui->stackedWidget_compareBackup->setCurrentIndex(0);

  ui->backButton->setAutoDefault(true);
  ui->backButton->setDefault(false);
  ui->nextButton->setAutoDefault(true);
  ui->nextButton->setDefault(true);

  ui->backButton_2->setAutoDefault(true);
  ui->backButton_2->setDefault(false);
  ui->nextButton_2->setAutoDefault(true);
  ui->nextButton_2->setDefault(true);

  ui->backButton_3->setAutoDefault(true);
  ui->backButton_3->setDefault(false);
  ui->nextButton_3->setAutoDefault(true);
  ui->nextButton_3->setDefault(true);

  ui->repoInfoLabel->setText("<NONE>");
  ui->repoInfoLabel_list->setText("<NONE>");
  ui->repoInfoLabel_cmp->setText("<NONE>");
  repository_ = nullptr;
  backup_ = nullptr;

  auto *header_list = ui->listTable->horizontalHeader(),
       *header_compare = ui->compareTable->horizontalHeader();

  header_list->setSectionResizeMode(0, QHeaderView::Fixed);
  header_list->setSectionResizeMode(1, QHeaderView::Fixed);
  header_list->setSectionResizeMode(2, QHeaderView::Fixed);
  header_list->setSectionResizeMode(3, QHeaderView::Stretch);

  header_compare->setSectionResizeMode(0, QHeaderView::Fixed);
  header_compare->setSectionResizeMode(1, QHeaderView::Fixed);
  header_compare->setSectionResizeMode(2, QHeaderView::Fixed);
  header_compare->setSectionResizeMode(3, QHeaderView::Stretch);

  ui->listTable->verticalHeader()->setVisible(false);
  ui->compareTable->verticalHeader()->setVisible(false);

  // Polished Features for Tables
  ui->listTable->setEditTriggers(QAbstractItemView::NoEditTriggers);
  ui->compareTable->setEditTriggers(QAbstractItemView::NoEditTriggers);

  ui->listTable->setSelectionMode(QAbstractItemView::NoSelection);
  ui->compareTable->setSelectionBehavior(QAbstractItemView::SelectRows);
  ui->compareTable->setSelectionMode(QAbstractItemView::MultiSelection);

  ui->listTable->setShowGrid(true);
  ui->compareTable->setShowGrid(true);
  ui->listTable->setFocusPolicy(Qt::NoFocus);
  ui->compareTable->setFocusPolicy(Qt::NoFocus);

  // Table Size
  QTimer::singleShot(0, this, [this]() { setColSize(ui->listTable->width()); });

  connect(ui->stackedWidget_createBackup, &QStackedWidget::currentChanged, this,
          &BackupTab::updateButtons);
  // connect(ui->compareTable->selectionModel(),
  //         &QItemSelectionModel::selectionChanged, this,
  //         &BackupTab::checkBackupSelection);

  updateButtons();
  updateProgress();
  checkRepoSelection();
}

BackupTab::~BackupTab() {
  delete ui;
  if (backup_) delete backup_;
  if (repository_) delete repository_;
}

void BackupTab::resizeEvent(QResizeEvent* event) {
  QWidget::resizeEvent(event);
  setColSize(ui->listTable->width());
}

void BackupTab::setColSize(int tableWidth) {
  int col_name = 300;
  int col_type = 200;
  int col_time = 300;
  int col_rem = tableWidth - col_name - col_type - col_time;

  ui->listTable->setColumnWidth(0, col_name);  // Name
  ui->listTable->setColumnWidth(1, col_type);  // Type
  ui->listTable->setColumnWidth(2, col_time);  // Time
  ui->listTable->setColumnWidth(3, col_rem);   // Remarks

  ui->compareTable->setColumnWidth(0, col_name);  // Name
  ui->compareTable->setColumnWidth(1, col_type);  // Type
  ui->compareTable->setColumnWidth(2, col_time);  // Time
  ui->compareTable->setColumnWidth(3, col_rem);   // Remarks
}

void BackupTab::fillListTable() {
  ui->listTable->clearContents();
  int backup_count = static_cast<int>(backup_list_.size());

  if (!backup_count) {
    ui->listTable->setRowCount(1);
    ui->listTable->setSpan(0, 0, 1, 4);
    auto* noBackups = new QTableWidgetItem("< No Backups Available >");
    noBackups->setTextAlignment(Qt::AlignCenter);
    noBackups->setForeground(Qt::darkGray);
    ui->listTable->setItem(0, 0, noBackups);
    return;
  }

  ui->listTable->setRowCount(backup_count);
  for (int row = 0; row < backup_count; ++row) {
    const BackupDetails& dtls = backup_list_[row];

    auto* nameItem = new QTableWidgetItem(QString::fromStdString(dtls.name));
    nameItem->setTextAlignment(Qt::AlignCenter);
    ui->listTable->setItem(row, 0, nameItem);

    auto* typeItem = new QTableWidgetItem(QString::fromStdString(dtls.type));
    typeItem->setTextAlignment(Qt::AlignCenter);
    ui->listTable->setItem(row, 1, typeItem);

    auto* timeItem =
        new QTableWidgetItem(QString::fromStdString(dtls.timestamp));
    timeItem->setTextAlignment(Qt::AlignCenter);
    ui->listTable->setItem(row, 2, timeItem);

    ui->listTable->setItem(
        row, 3, new QTableWidgetItem(QString::fromStdString(dtls.remarks)));
  }
}

void BackupTab::fillCompareTable() {
  ui->compareTable->clearContents();
  int backup_count = static_cast<int>(backup_list_.size());

  if (!backup_count) {
    ui->compareTable->setRowCount(1);
    ui->compareTable->setSpan(0, 0, 1, 4);
    auto* noBackups = new QTableWidgetItem("< No Backups Available >");
    noBackups->setTextAlignment(Qt::AlignCenter);
    noBackups->setForeground(Qt::darkGray);
    ui->compareTable->setItem(0, 0, noBackups);
    return;
  }

  ui->compareTable->setRowCount(backup_count);
  for (int row = 0; row < backup_count; ++row) {
    const BackupDetails& dtls = backup_list_[row];

    auto* nameItem = new QTableWidgetItem(QString::fromStdString(dtls.name));
    nameItem->setTextAlignment(Qt::AlignCenter);
    ui->compareTable->setItem(row, 0, nameItem);

    auto* typeItem = new QTableWidgetItem(QString::fromStdString(dtls.type));
    typeItem->setTextAlignment(Qt::AlignCenter);
    ui->compareTable->setItem(row, 1, typeItem);

    auto* timeItem =
        new QTableWidgetItem(QString::fromStdString(dtls.timestamp));
    timeItem->setTextAlignment(Qt::AlignCenter);
    ui->compareTable->setItem(row, 2, timeItem);

    ui->compareTable->setItem(
        row, 3, new QTableWidgetItem(QString::fromStdString(dtls.remarks)));
  }
}

void BackupTab::on_createBackupButton_clicked() {
  repository_ = nullptr;
  ui->repoInfoLabel->setText("<NONE>");
  ui->stackedWidget->setCurrentIndex(1);
  ui->stackedWidget_createBackup->setCurrentIndex(0);
  checkRepoSelection();
  updateProgress();
}

void BackupTab::on_listBackupButton_clicked() {
  repository_ = nullptr;
  ui->repoInfoLabel_list->setText("<NONE>");
  ui->stackedWidget->setCurrentIndex(2);
  ui->stackedWidget_listBackup->setCurrentIndex(0);
  checkRepoSelection();
  updateProgress();
}

void BackupTab::on_compareBackupButton_clicked() {
  repository_ = nullptr;
  ui->repoInfoLabel_cmp->setText("<NONE>");
  ui->stackedWidget->setCurrentIndex(3);
  ui->stackedWidget_compareBackup->setCurrentIndex(0);
  checkRepoSelection();
  updateProgress();
}

void BackupTab::updateProgress() {
  switch (ui->stackedWidget->currentIndex()) {
    case 1: {
      ui->progressBar->setValue(
          ((ui->stackedWidget_createBackup->currentIndex() + 1) * 100) /
          ui->stackedWidget_createBackup->count());
      break;
    }
    case 2: {
      ui->progressBar_2->setValue(
          ((ui->stackedWidget_listBackup->currentIndex() + 1) * 100) /
          ui->stackedWidget_listBackup->count());
      break;
    }
    case 3: {
      ui->progressBar_3->setValue(
          ((ui->stackedWidget_compareBackup->currentIndex() + 1) * 100) /
          ui->stackedWidget_compareBackup->count());
      break;
    }
    default: {
      break;
    }
  }
}

void BackupTab::updateButtons() {
  switch (ui->stackedWidget->currentIndex()) {
    case 1: {
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
      break;
    }
    case 2: {
      int index = ui->stackedWidget_listBackup->currentIndex();
      int total = ui->stackedWidget_listBackup->count();
      if (index == 0) {
        // ui->backButton_2->setEnabled(false);
      } else {
        ui->backButton_2->setEnabled(true);
      }
      if (index == total - 1) {
        ui->nextButton_2->setVisible(false);
      } else {
        ui->nextButton_2->setVisible(true);
      }
      break;
    }
    case 3: {
      int index = ui->stackedWidget_compareBackup->currentIndex();
      int total = ui->stackedWidget_compareBackup->count();
      if (index == 0) {
        // ui->backButton_3->setEnabled(false);
      } else {
        ui->backButton_3->setEnabled(true);
      }
      if (index == total - 1) {
        ui->nextButton_3->setText("Compare");
      } else {
        ui->nextButton_3->setText("Next");
      }
      break;
    }
    default:
      break;
  }
}

void BackupTab::checkRepoSelection() {
  ui->nextButton->setEnabled(repository_ != nullptr);
  ui->nextButton_2->setEnabled(repository_ != nullptr);
  ui->nextButton_3->setEnabled(repository_ != nullptr);
}

void BackupTab::checkBackupSelection() {
  // QModelIndexList selected =
  // ui->compareTable->selectionModel()->selectedRows();

  // bool validSelection = !selected.isEmpty() && selected.first().row() != 0;
  // ui->nextButton_3->setEnabled(validSelection);
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

void BackupTab::on_backButton_2_clicked() {
  int index = ui->stackedWidget_listBackup->currentIndex();
  if (index > 0) {
    ui->stackedWidget_listBackup->setCurrentIndex(index - 1);
    ui->nextButton_2->setHidden(false);
  } else {
    ui->stackedWidget->setCurrentIndex(0);
  }
  updateProgress();
}

void BackupTab::on_backButton_3_clicked() {
  int index = ui->stackedWidget_compareBackup->currentIndex();
  if (index > 0) {
    ui->stackedWidget_compareBackup->setCurrentIndex(index - 1);
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

void BackupTab::on_nextButton_2_clicked() {
  int index = ui->stackedWidget_listBackup->currentIndex();
  int total = ui->stackedWidget_listBackup->count();

  if (!handleSelectRepo()) {
    return;
  }

  if (index == 0) {
    listBackups();
  }
  updateProgress();
}

void BackupTab::on_nextButton_3_clicked() {
  int index = ui->stackedWidget_compareBackup->currentIndex();
  int total = ui->stackedWidget_compareBackup->count();

  bool valid = true;
  switch (index) {
    case 0:
      valid = handleSelectRepo();
      break;
    case 1:
      valid = handleBackupDetails();
      break;
    default:
      break;
  }
  if (!valid) {
    return;
  }

  if (index < total - 1) {
    ui->stackedWidget_compareBackup->setCurrentIndex(index + 1);
  } else {
    ui->nextButton_3->setEnabled(false);
    compareBackups();
  }
  updateProgress();
}

void BackupTab::on_createRepoButton_clicked() {
  if (repository_) {
    delete repository_;
    repository_ = nullptr;
  }
  CreateRepositoryDialog dialog(this);
  dialog.setWindowFlags(Qt::Window);
  if (dialog.exec() == QDialog::Accepted) {
    repository_ = dialog.getRepository();
    ui->repoInfoLabel->setText(
        QString::fromStdString(repository_->GetRepositoryInfoString()));
  } else {
    repository_ = nullptr;
    ui->repoInfoLabel->setText("<NONE>");
  }
  checkRepoSelection();
}

void BackupTab::on_useRepoButton_clicked() {
  if (repository_) {
    delete repository_;
    repository_ = nullptr;
  }
  UseRepositoryDialog dialog(this);
  dialog.setWindowFlags(Qt::Window);
  if (dialog.exec() == QDialog::Accepted) {
    repository_ = dialog.getRepository();
    ui->repoInfoLabel->setText(
        QString::fromStdString(repository_->GetRepositoryInfoString()));
  } else {
    repository_ = nullptr;
    ui->repoInfoLabel->setText("<NONE>");
  }
  checkRepoSelection();
}

void BackupTab::on_chooseRepoButton_list_clicked() {
  if (repository_) {
    delete repository_;
    repository_ = nullptr;
  }
  UseRepositoryDialog dialog(this);
  dialog.setWindowFlags(Qt::Window);
  dialog.setWindowTitle("Choose Repository for Listing Backups");
  if (dialog.exec() == QDialog::Accepted) {
    repository_ = dialog.getRepository();
    ui->repoInfoLabel_list->setText(
        QString::fromStdString(repository_->GetRepositoryInfoString()));
  } else {
    repository_ = nullptr;
    ui->repoInfoLabel_list->setText("<NONE>");
  }
  checkRepoSelection();
}

void BackupTab::on_chooseRepoButton_compare_clicked() {
  if (repository_) {
    delete repository_;
    repository_ = nullptr;
  }
  UseRepositoryDialog dialog(this);
  dialog.setWindowFlags(Qt::Window);
  dialog.setWindowTitle("Choose Repository for Comparing Backups");
  if (dialog.exec() == QDialog::Accepted) {
    repository_ = dialog.getRepository();
    ui->repoInfoLabel_cmp->setText(
        QString::fromStdString(repository_->GetRepositoryInfoString()));
  } else {
    repository_ = nullptr;
    ui->repoInfoLabel_cmp->setText("<NONE>");
  }
  checkRepoSelection();
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
  if (!fs::exists(source_path_)) {
    MessageBoxDecorator::showMessageBox(this, "Invalid Input",
                                        "Source path does not exist.",
                                        QMessageBox::Warning);
    return false;
  }

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

bool BackupTab::handleSelectBackups() {
  return true;  // TODO: Implement backup selection logic
}

bool BackupTab::handleSchedule() {
  bool schedule = ui->yesSchButton->isChecked();
  if (schedule) {
    // TODO: Schedule
  }
  return true;
}

void BackupTab::initBackup() {
  try {
    if (backup_) {
      delete backup_;
      backup_ = nullptr;
    }

    backup_ =
        new BackupGUI(this, repository_, source_path_, backup_type_, remarks_);

  } catch (const std::exception& e) {
    MessageBoxDecorator::showMessageBox(this, "Backup Initialization Failure",
                                        QString::fromStdString(e.what()),
                                        QMessageBox::Critical);
    Logger::SystemLog(
        "GUI | Failed to initialize backup: " + std::string(e.what()),
        LogLevel::ERROR);

    ui->nextButton->setEnabled(true);
    return;
  }

  try {
    backup_->BackupDirectory([this](bool success) {
      if (success) {
        ui->stackedWidget->setCurrentIndex(0);
        checkRepoSelection();
      } else {
        ui->nextButton->setEnabled(true);
      }
    });
  } catch (const std::exception& e) {
    MessageBoxDecorator::showMessageBox(this, "Backup Finalization Failure",
                                        QString::fromStdString(e.what()),
                                        QMessageBox::Critical);
    Logger::SystemLog(
        "GUI | Failed to finalize backup: " + std::string(e.what()),
        LogLevel::ERROR);

    ui->nextButton->setEnabled(true);
  }
}

void BackupTab::listBackups() {
  try {
    if (backup_) {
      delete backup_;
      backup_ = nullptr;
    }
    source_path_ = "/";
    backup_type_ = BackupType::FULL;
    remarks_ = "";

    backup_ =
        new BackupGUI(this, repository_, source_path_, backup_type_, remarks_);

    ProgressBoxDecorator::runProgressBoxIndeterminate(
        this,
        [&](std::function<void(const QString&)> setWaitMessage,
            std::function<void(const QString&)> setSuccessMessage,
            std::function<void(const QString&)> setFailureMessage) -> bool {
          try {
            setWaitMessage("Fetching backups...");
            backup_list_ = backup_->GetAllBackupDetails();
            fillListTable();

            Logger::SystemLog("GUI | Backups fetched from repository : " +
                              repository_->GetRepositoryInfoString());

            setSuccessMessage(
                "Backups fetched from repository: " +
                QString::fromStdString(repository_->GetRepositoryInfoString()));
            return true;

          } catch (const std::exception& e) {
            Logger::SystemLog(
                "GUI | Cannot fetch backups: " + std::string(e.what()),
                LogLevel::ERROR);
            setFailureMessage("Backup fetching failed: " +
                              QString::fromStdString(e.what()));
            return false;
          }
        },
        "Fetching backups...", "Backups fetched successfully.",
        "Backup fetching failed.",
        [&](bool success) {
          if (success) {
            ui->nextButton_2->setHidden(true);
            ui->stackedWidget_listBackup->setCurrentIndex(1);
          } else {
            ui->nextButton->setHidden(false);
          }
        });

  } catch (const std::exception& e) {
    MessageBoxDecorator::showMessageBox(this, "Backup Listing Failure",
                                        QString::fromStdString(e.what()),
                                        QMessageBox::Critical);
    Logger::SystemLog("GUI | Failed to list backups: " + std::string(e.what()),
                      LogLevel::ERROR);

    return;
  }
}

void BackupTab::compareBackups() {}
