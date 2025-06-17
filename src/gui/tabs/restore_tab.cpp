#include "gui/tabs/restore_tab.h"

#include "gui/tabs/ui_restore_tab.h"
#include "gui/decorators/message_box.h"
#include "gui/dialog/use_repository_dialog.h"

RestoreTab::RestoreTab(QWidget *parent)
    : QWidget(parent), ui(new Ui::RestoreTab) {
  ui->setupUi(this);
  ui->stackedWidget->setCurrentIndex(0);

  connect(ui->stackedWidget_attemptRestore, &QStackedWidget::currentChanged, this,
            &RestoreTab::updateButtons);
}

RestoreTab::~RestoreTab() { delete ui; }

void RestoreTab::on_restoreButton_clicked()
{
    ui->stackedWidget->setCurrentIndex(1);
}

void RestoreTab::on_retryButton_clicked()
{
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
  if (index < total - 1) {
    ui->stackedWidget_attemptRestore->setCurrentIndex(index + 1);
  } else {
    // TODO: Trigger Restore Process
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

void RestoreTab::on_chooseRepoButton_clicked()
{
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
