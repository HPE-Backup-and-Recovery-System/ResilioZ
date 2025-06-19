#include "gui/tabs/restore_tab.h"

#include <QDateTime>
#include <QDir>
#include <QFileDialog>
#include <QFileInfo>
#include <QFileInfoList>
#include <QTimer>

#include "gui/decorators/message_box.h"
#include "gui/dialog/use_repository_dialog.h"
#include "gui/tabs/ui_restore_tab.h"

RestoreTab::RestoreTab(QWidget* parent)
    : QWidget(parent), ui(new Ui::RestoreTab) {
  ui->setupUi(this);

  ui->stackedWidget->setCurrentIndex(0);
  ui->backButton->setAutoDefault(true);
  ui->backButton->setDefault(false);
  ui->nextButton->setAutoDefault(true);
  ui->nextButton->setDefault(true);

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

void RestoreTab::on_chooseRepoButton_clicked() {
  UseRepositoryDialog dialog(this);
  dialog.setWindowFlags(Qt::Window);
  if (dialog.exec() == QDialog::Accepted) {
    repository_ = nullptr;  // TODO...
    MessageBoxDecorator::showMessageBox(this, "Success", "Repository selected.",
                                        QMessageBox::Information);
  } else {
    repository_ = nullptr;
    MessageBoxDecorator::showMessageBox(
        this, "Error", "Repository not selected.", QMessageBox::Warning);
  }
}

void RestoreTab::onAttemptRestorePageChanged(int index) {
  updateButtons();
  if (index == 1) {
    QTimer::singleShot(0, this, [this]() { loadFileTable(); });
  }
}

void RestoreTab::loadFileTable() {
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

  // To do: Take input of previous step here
  QString directoryPath = "/home/user/Desktop/HPE/NykajRepo/backup";
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
    QString fileName = selectedItems.first()->text();
    // To do
  } else {
    // To do
  }
}

void RestoreTab::on_chooseDestination_clicked() {
  QString selectedDir = QFileDialog::getExistingDirectory(
      this, "Choose Backup Directory", QDir::homePath(),
      QFileDialog::ShowDirsOnly | QFileDialog::DontResolveSymlinks);
  if (!selectedDir.isEmpty()) {
    // To do
  }
}
