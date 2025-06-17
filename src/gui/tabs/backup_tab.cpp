#include "gui/tabs/backup_tab.h"

#include "gui/decorators/message_box.h"
#include "gui/dialog/create_repository_dialog.h"
#include "gui/dialog/use_repository_dialog.h"
#include "gui/tabs/ui_backup_tab.h"

BackupTab::BackupTab(QWidget *parent) : QWidget(parent), ui(new Ui::BackupTab) {
  ui->setupUi(this);
  ui->stackedWidget->setCurrentIndex(0);

  connect(ui->stackedWidget_createBackup, &QStackedWidget::currentChanged, this,
          &BackupTab::updateButtons);

  updateButtons();
}

BackupTab::~BackupTab() { delete ui; }

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

void BackupTab::on_nextButton_clicked() {
  int index = ui->stackedWidget_createBackup->currentIndex();
  int total = ui->stackedWidget_createBackup->count();
  if (index < total - 1) {
    ui->stackedWidget_createBackup->setCurrentIndex(index + 1);
  } else {
    // TODO: Trigger Backup Process
  }
  updateProgress();
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

void BackupTab::on_createRepoButton_clicked() {
  CreateRepositoryDialog dialog(this);
  dialog.setWindowFlags(Qt::Window);
  if (dialog.exec() == QDialog::Accepted) {
    repository_ = nullptr;  // TODO...
    MessageBoxDecorator::ShowMessageBox(this, "Success", "Repository created.",
                                        QMessageBox::Information);
  } else {
    repository_ = nullptr;
    MessageBoxDecorator::ShowMessageBox(
        this, "Error", "Repository not created.", QMessageBox::Warning);
  }
}

void BackupTab::on_useRepoButton_clicked() {
    UseRepositoryDialog dialog(this);
    dialog.setWindowFlags(Qt::Window);
    if (dialog.exec() == QDialog::Accepted) {
        repository_ = nullptr;  // TODO...
        MessageBoxDecorator::ShowMessageBox(this, "Success", "Repository selected.",
                                            QMessageBox::Information);
    } else {
        repository_ = nullptr;
        MessageBoxDecorator::ShowMessageBox(
            this, "Error", "Repository not selected.", QMessageBox::Warning);
    }
}
