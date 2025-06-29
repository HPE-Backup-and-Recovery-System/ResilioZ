#include "gui/dialog/backup_verification_dialog.h"

#include "gui/dialog/ui_backup_verification_dialog.h"

BackupVerificationDialog::BackupVerificationDialog(
    QWidget* parent, const std::vector<std::string>& success_files,
    const std::vector<std::string>& corrupt_files,
    const std::vector<std::string>& fail_files, const std::string& window_title,
    const std::string& title_label)
    : QDialog(parent),
      ui(new Ui::BackupVerificationDialog),
      success_files_(success_files),
      corrupt_files_(corrupt_files),
      fail_files_(fail_files) {

  ui->setupUi(this);

  this->setWindowTitle(QString::fromStdString(window_title));
  ui->titleLabel->setText(QString::fromStdString(title_label));

  fillTables();

  auto* header_success = ui->successFileTable->horizontalHeader();
  header_success->setSectionResizeMode(QHeaderView::Stretch);
  ui->successFileTable->verticalHeader()->setVisible(false);

  auto* header_corrupt = ui->corruptFileTable->horizontalHeader();
  header_corrupt->setSectionResizeMode(QHeaderView::Stretch);
  ui->corruptFileTable->verticalHeader()->setVisible(false);

  auto* header_fail = ui->failFileTable->horizontalHeader();
  header_fail->setSectionResizeMode(QHeaderView::Stretch);
  ui->failFileTable->verticalHeader()->setVisible(false);

  ui->successFileTable->setSelectionMode(QAbstractItemView::NoSelection);
  ui->successFileTable->setEditTriggers(QAbstractItemView::NoEditTriggers);
  ui->successFileTable->setShowGrid(true);
  ui->successFileTable->setFocusPolicy(Qt::NoFocus);

  ui->corruptFileTable->setSelectionMode(QAbstractItemView::NoSelection);
  ui->corruptFileTable->setEditTriggers(QAbstractItemView::NoEditTriggers);
  ui->corruptFileTable->setShowGrid(true);
  ui->corruptFileTable->setFocusPolicy(Qt::NoFocus);

  ui->failFileTable->setSelectionMode(QAbstractItemView::NoSelection);
  ui->failFileTable->setEditTriggers(QAbstractItemView::NoEditTriggers);
  ui->failFileTable->setShowGrid(true);
  ui->failFileTable->setFocusPolicy(Qt::NoFocus);
}

BackupVerificationDialog::~BackupVerificationDialog() { delete ui; }

void BackupVerificationDialog::fillTables() {
  ui->successFileTable->clearContents();
  ui->corruptFileTable->clearContents();
  ui->failFileTable->clearContents();

  int success_count = static_cast<int>(success_files_.size());
  int corrupt_count = static_cast<int>(corrupt_files_.size());
  int fail_count = static_cast<int>(fail_files_.size());

  if (!success_count) {
    ui->successFileTable->setRowCount(1);
    auto* noFile = new QTableWidgetItem("< No Files Succeeded >");
    noFile->setTextAlignment(Qt::AlignCenter);
    noFile->setForeground(Qt::darkGray);
    ui->successFileTable->setItem(0, 0, noFile);
  } else {
    ui->successFileTable->setRowCount(success_count);
    for (int row = 0; row < success_count; ++row) {
      auto* fileItem =
          new QTableWidgetItem(QString::fromStdString(success_files_[row]));
      fileItem->setTextAlignment(Qt::AlignRight);
      ui->successFileTable->setItem(row, 0, fileItem);
    }
  }

  if (!corrupt_count) {
    ui->corruptFileTable->setRowCount(1);
    auto* noFile = new QTableWidgetItem("< No Files Corrupt >");
    noFile->setTextAlignment(Qt::AlignCenter);
    noFile->setForeground(Qt::darkGray);
    ui->corruptFileTable->setItem(0, 0, noFile);
  } else {
    ui->corruptFileTable->setRowCount(corrupt_count);
    for (int row = 0; row < corrupt_count; ++row) {
      auto* fileItem =
          new QTableWidgetItem(QString::fromStdString(corrupt_files_[row]));
      fileItem->setTextAlignment(Qt::AlignRight);
    
      ui->corruptFileTable->setItem(row, 0, fileItem);
    }
  }

  if (!fail_count) {
    ui->failFileTable->setRowCount(1);
    auto* noFile = new QTableWidgetItem("< No Files Failed >");
    noFile->setTextAlignment(Qt::AlignCenter);
    noFile->setForeground(Qt::darkGray);
    ui->failFileTable->setItem(0, 0, noFile);
  } else {
    ui->failFileTable->setRowCount(fail_count);
    for (int row = 0; row < fail_count; ++row) {
      auto* fileItem =
          new QTableWidgetItem(QString::fromStdString(fail_files_[row]));
      fileItem->setTextAlignment(Qt::AlignRight);
      ui->failFileTable->setItem(row, 0, fileItem);
    }
  }
}

void BackupVerificationDialog::on_backButton_clicked() { accept(); }
