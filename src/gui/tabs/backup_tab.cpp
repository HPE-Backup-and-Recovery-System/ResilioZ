#include "gui/tabs/backup_tab.h"

#include <QModelIndexList>
#include <QThread>
#include <QTimer>
#include <iostream>

#include "gui/decorators/core.h"
#include "gui/decorators/message_box.h"
#include "gui/decorators/progress_box.h"
#include "gui/dialog/attach_schedule_dialog.h"
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

  request_mgr = new SchedulerRequestManager();

  auto* header_list = ui->listTable->horizontalHeader();

  header_list->setSectionResizeMode(0, QHeaderView::Fixed);
  header_list->setSectionResizeMode(1, QHeaderView::Fixed);
  header_list->setSectionResizeMode(2, QHeaderView::Fixed);
  header_list->setSectionResizeMode(3, QHeaderView::Stretch);

  auto* header_compare = ui->firstCmpTable->horizontalHeader();

  header_compare->setSectionResizeMode(0, QHeaderView::Fixed);
  header_compare->setSectionResizeMode(1, QHeaderView::Fixed);
  header_compare->setSectionResizeMode(2, QHeaderView::Fixed);
  header_compare->setSectionResizeMode(3, QHeaderView::Stretch);

  header_compare = ui->secondCmpTable->horizontalHeader();

  header_compare->setSectionResizeMode(0, QHeaderView::Fixed);
  header_compare->setSectionResizeMode(1, QHeaderView::Fixed);
  header_compare->setSectionResizeMode(2, QHeaderView::Fixed);
  header_compare->setSectionResizeMode(3, QHeaderView::Stretch);

  ui->listTable->verticalHeader()->setVisible(false);
  ui->firstCmpTable->verticalHeader()->setVisible(false);
  ui->secondCmpTable->verticalHeader()->setVisible(false);

  // Polished Features for Tables
  ui->listTable->setEditTriggers(QAbstractItemView::NoEditTriggers);
  ui->listTable->setSelectionMode(QAbstractItemView::NoSelection);
  ui->listTable->setShowGrid(true);
  ui->listTable->setFocusPolicy(Qt::NoFocus);

  ui->firstCmpTable->setEditTriggers(QAbstractItemView::NoEditTriggers);
  ui->firstCmpTable->setSelectionBehavior(QAbstractItemView::SelectRows);
  ui->firstCmpTable->setSelectionMode(QAbstractItemView::SingleSelection);
  ui->firstCmpTable->setShowGrid(true);
  ui->firstCmpTable->setFocusPolicy(Qt::NoFocus);

  ui->secondCmpTable->setEditTriggers(QAbstractItemView::NoEditTriggers);
  ui->secondCmpTable->setSelectionBehavior(QAbstractItemView::SelectRows);
  ui->secondCmpTable->setSelectionMode(QAbstractItemView::SingleSelection);
  ui->secondCmpTable->setShowGrid(true);
  ui->secondCmpTable->setFocusPolicy(Qt::NoFocus);

  // Table Size
  QTimer::singleShot(0, this, [this]() { setColSize(ui->listTable->width()); });

  connect(ui->stackedWidget_createBackup, &QStackedWidget::currentChanged, this,
          &BackupTab::updateButtons);

  connect(ui->firstCmpTable->selectionModel(),
          &QItemSelectionModel::selectionChanged, this,
          &BackupTab::checkBackupSelection);
  connect(ui->secondCmpTable->selectionModel(),
          &QItemSelectionModel::selectionChanged, this,
          &BackupTab::checkBackupSelection);

  updateButtons();
  updateProgress();
  checkRepoSelection();
}

BackupTab::~BackupTab() {
  delete ui;
  if (backup_ != nullptr) delete backup_;
  if (repository_ != nullptr) delete repository_;
  if (request_mgr != nullptr) delete request_mgr;
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

  // List Backup Table

  ui->listTable->setColumnWidth(0, col_name);  // Name
  ui->listTable->setColumnWidth(1, col_type);  // Type
  ui->listTable->setColumnWidth(2, col_time);  // Time
  ui->listTable->setColumnWidth(3, col_rem);   // Remarks

  // Compare Backup Tables

  ui->firstCmpTable->setColumnWidth(0, col_name);  // Name
  ui->firstCmpTable->setColumnWidth(1, col_type);  // Type
  ui->firstCmpTable->setColumnWidth(2, col_time);  // Time
  ui->firstCmpTable->setColumnWidth(3, col_rem);   // Remarks

  ui->secondCmpTable->setColumnWidth(0, col_name);  // Name
  ui->secondCmpTable->setColumnWidth(1, col_type);  // Type
  ui->secondCmpTable->setColumnWidth(2, col_time);  // Time
  ui->secondCmpTable->setColumnWidth(3, col_rem);   // Remarks
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
  int backup_count = static_cast<int>(backup_list_.size());

  ui->firstCmpTable->clearContents();
  ui->secondCmpTable->clearContents();

  ui->firstCmpTable->setRowCount(backup_count + 1);
  ui->secondCmpTable->setRowCount(backup_count + 1);

  ui->firstCmpTable->setSpan(0, 0, 1, 4);
  ui->secondCmpTable->setSpan(0, 0, 1, 4);

  auto* noSelection = new QTableWidgetItem("< No Selection >");
  noSelection->setTextAlignment(Qt::AlignCenter);
  noSelection->setForeground(Qt::darkGray);

  ui->firstCmpTable->setItem(0, 0, noSelection);
  ui->secondCmpTable->setItem(0, 0, noSelection);

  for (int i = 0; i < backup_count; ++i) {
    const BackupDetails& dtls = backup_list_[i];
    int row = i + 1;

    auto *nameItem_1 = new QTableWidgetItem(QString::fromStdString(dtls.name)),
         *nameItem_2 = new QTableWidgetItem(QString::fromStdString(dtls.name));

    nameItem_1->setTextAlignment(Qt::AlignCenter);
    nameItem_2->setTextAlignment(Qt::AlignCenter);
    ui->firstCmpTable->setItem(row, 0, nameItem_1);
    ui->secondCmpTable->setItem(row, 0, nameItem_2);

    auto *typeItem_1 = new QTableWidgetItem(QString::fromStdString(dtls.type)),
         *typeItem_2 = new QTableWidgetItem(QString::fromStdString(dtls.type));

    typeItem_1->setTextAlignment(Qt::AlignCenter);
    typeItem_2->setTextAlignment(Qt::AlignCenter);
    ui->firstCmpTable->setItem(row, 1, typeItem_1);
    ui->secondCmpTable->setItem(row, 1, typeItem_2);

    auto *timeItem_1 =
             new QTableWidgetItem(QString::fromStdString(dtls.timestamp)),
         *timeItem_2 =
             new QTableWidgetItem(QString::fromStdString(dtls.timestamp));

    timeItem_1->setTextAlignment(Qt::AlignCenter);
    timeItem_2->setTextAlignment(Qt::AlignCenter);
    ui->firstCmpTable->setItem(row, 2, timeItem_1);
    ui->secondCmpTable->setItem(row, 2, timeItem_2);

    auto *remarksItem_1 =
             new QTableWidgetItem(QString::fromStdString(dtls.remarks)),
         *remarksItem_2 =
             new QTableWidgetItem(QString::fromStdString(dtls.remarks));

    ui->firstCmpTable->setItem(row, 3, remarksItem_1);
    ui->secondCmpTable->setItem(row, 3, remarksItem_2);
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
  QModelIndexList selected_first =
                      ui->firstCmpTable->selectionModel()->selectedRows(),
                  selected_second =
                      ui->secondCmpTable->selectionModel()->selectedRows();

  bool validSelection =
      !selected_first.isEmpty() && selected_first.first().row() != 0 &&
      !selected_second.isEmpty() && selected_second.first().row() != 0;
  ui->nextButton_3->setEnabled(validSelection);
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
      valid = handleSelectBackups();
      break;
    default:
      break;
  }
  if (!valid) {
    return;
  }

  switch (index) {
    case 0: {
      listBackups();
      break;
    }
    case 1: {
      ui->nextButton_3->setEnabled(false);
      compareBackups();
      break;
    }
    default:
      break;
  }
  updateProgress();
}

void BackupTab::on_createRepoButton_clicked() {
  if (repository_ != nullptr) {
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
  if (repository_ != nullptr) {
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
  if (repository_ != nullptr) {
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
  if (repository_ != nullptr) {
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
  QModelIndexList selected_first =
                      ui->firstCmpTable->selectionModel()->selectedRows(),
                  selected_second =
                      ui->secondCmpTable->selectionModel()->selectedRows();
  if (selected_first.isEmpty() || selected_second.isEmpty()) {
    MessageBoxDecorator::showMessageBox(this, "Backup Selection Error",
                                        "Please select two backups to compare.",
                                        QMessageBox::Warning);
    return false;
  }

  if (selected_first.first().row() == 0) {
    MessageBoxDecorator::showMessageBox(
        this, "Backup Not Selected", "Please select first backup to compare.",
        QMessageBox::Warning);
    return false;
  }
  if (selected_second.first().row() == 0) {
    MessageBoxDecorator::showMessageBox(
        this, "Backup Not Selected", "Please select second backup to compare.",
        QMessageBox::Warning);
    return false;
  }

  cmp_first_backup_ = backup_list_[selected_first.first().row() - 1].name;
  cmp_second_backup_ = backup_list_[selected_second.first().row() - 1].name;
  return true;
}

bool BackupTab::handleSchedule() {
  bool schedule = ui->yesSchButton->isChecked();
  if (schedule) {
    ui->noSchButton->setChecked(false);
    ui->yesSchButton->setChecked(true);
  } else {
    ui->noSchButton->setChecked(true);
    ui->yesSchButton->setChecked(false);
  }
  return true;
}

void BackupTab::initBackup() {
  try {
    if (backup_ != nullptr) {
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
    if (backup_ != nullptr) {
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
            fillCompareTable();

            Logger::SystemLog("GUI | Backups fetched from repository: " +
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
          int index = ui->stackedWidget->currentIndex();

          switch (index) {
            case 2: {
              // List Backups Page
              if (success) {
                ui->nextButton_2->setHidden(true);
                ui->stackedWidget_listBackup->setCurrentIndex(1);
                updateProgress();
              } else {
                ui->nextButton_2->setHidden(false);
              }
              break;
            }
            case 3: {
              // Compare Backups Page
              if (success) {
                ui->nextButton_3->setEnabled(false);
                ui->stackedWidget_compareBackup->setCurrentIndex(1);
                updateProgress();
              } else {
                ui->nextButton_3->setEnabled(true);
              }
              break;
            }
            default:
              break;
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

void BackupTab::compareBackups() {
  try {
    if (backup_ != nullptr) {
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
          setWaitMessage("Comparing backups...");
          std::string comparison =
              backup_->CompareBackups(cmp_first_backup_, cmp_second_backup_);

          setSuccessMessage(QString::fromStdString(comparison));
          return true;
        },
        "Comparing backups...", "Backups compared successfully.",
        "Backup comparison failed.",
        [&](bool success) { ui->nextButton_3->setEnabled(true); });

  } catch (const std::exception& e) {
    MessageBoxDecorator::showMessageBox(this, "Backup Comparison Failure",
                                        QString::fromStdString(e.what()),
                                        QMessageBox::Critical);
    Logger::SystemLog(
        "GUI | Failed to compare backups: " + std::string(e.what()),
        LogLevel::ERROR);

    return;
  }
}

void BackupTab::on_yesSchButton_clicked() {
  std::string destination_path_ = repository_->GetFullPath();
  AttachScheduleDialog dialog(this, source_path_, destination_path_, remarks_,
                              backup_type_);
  dialog.setWindowFlags(Qt::Window);
  if (dialog.exec() == QDialog::Accepted) {
    std::string schedule = dialog.getSchedule();

    std::string schedule_id = request_mgr->SendAddRequest(
        schedule, source_path_, repository_->GetName(), repository_->GetPath(),
        repository_->GetPassword(), "", repository_->GetType(), remarks_,
        backup_type_);

    QString success_message = QString::fromStdString(
        "Schedule: " + schedule_id + " created successfully.");
    MessageBoxDecorator::showMessageBox(this, "Success", success_message,
                                        QMessageBox::Information);

  } else {
    ui->noSchButton->setChecked(true);
    ui->yesSchButton->setChecked(false);
  }
}
