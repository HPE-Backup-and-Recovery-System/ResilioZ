#include "gui/dialog/use_repository_dialog.h"

#include <QModelIndexList>
#include <QThread>
#include <QTimer>

#include "gui/decorators/message_box.h"
#include "gui/decorators/progress_box.h"
#include "gui/dialog/ui_use_repository_dialog.h"
#include "repositories/repository.h"
#include "utils/utils.h"

UseRepositoryDialog::UseRepositoryDialog(QWidget* parent)
    : QDialog(parent), ui(new Ui::UseRepositoryDialog) {
  ui->setupUi(this);

  repodata_mgr_ = new RepodataManager();
  repos = repodata_mgr_->GetAll();
  FillTable();

  auto* header = ui->repoTable->horizontalHeader();
  header->setSectionResizeMode(QHeaderView::Fixed);
  ui->repoTable->verticalHeader()->setVisible(false);

  // Polished Features for Table
  ui->repoTable->setEditTriggers(QAbstractItemView::NoEditTriggers);
  ui->repoTable->setSelectionBehavior(QAbstractItemView::SelectRows);
  ui->repoTable->setSelectionMode(QAbstractItemView::SingleSelection);
  ui->repoTable->setShowGrid(true);
  ui->repoTable->setFocusPolicy(Qt::NoFocus);

  // Table Size
  QTimer::singleShot(
      0, this, [this]() { SetColSize(ui->repoTable->viewport()->width()); });
}

UseRepositoryDialog::~UseRepositoryDialog() {
  delete ui;
  delete repodata_mgr_;
}

void UseRepositoryDialog::resizeEvent(QResizeEvent* event) {
  QDialog::resizeEvent(event);  // QWidget::resizeEvent(event); for QWidgets
  SetColSize(ui->repoTable->viewport()->width());
}

void UseRepositoryDialog::SetColSize(int tableWidth) {
  int col_created_at = 200;
  int col_name = 240;
  int col_type = 100;
  int col_path = tableWidth - col_created_at - col_name - col_type;

  ui->repoTable->setColumnWidth(0, col_created_at);  // Created At
  ui->repoTable->setColumnWidth(1, col_name);        // Name
  ui->repoTable->setColumnWidth(2, col_type);        // Type
  ui->repoTable->setColumnWidth(3, col_path);        // Path
}

void UseRepositoryDialog::FillTable() {
  ui->repoTable->clearContents();
  ui->repoTable->setRowCount(static_cast<int>(repos.size()) + 1);

  ui->repoTable->setSpan(0, 0, 1, 4);
  auto* noSelection = new QTableWidgetItem("< No Selection >");
  noSelection->setTextAlignment(Qt::AlignCenter);
  noSelection->setForeground(Qt::darkGray);
  ui->repoTable->setItem(0, 0, noSelection);

  for (int i = 0; i < static_cast<int>(repos.size()); ++i) {
    const RepoEntry& repo = repos[i];
    int row = i + 1;

    auto* createdItem =
        new QTableWidgetItem(QString::fromStdString(repo.created_at));
    createdItem->setTextAlignment(Qt::AlignCenter);
    ui->repoTable->setItem(row, 0, createdItem);

    ui->repoTable->setItem(
        row, 1, new QTableWidgetItem(QString::fromStdString(repo.name)));

    auto* typeItem =
        new QTableWidgetItem(QString::fromStdString(repo.type).toUpper());
    typeItem->setTextAlignment(Qt::AlignCenter);
    ui->repoTable->setItem(row, 2, typeItem);

    ui->repoTable->setItem(
        row, 3, new QTableWidgetItem(QString::fromStdString(repo.path)));
  }

  ui->repoTable->selectRow(0);
}

void UseRepositoryDialog::on_backButton_clicked() { reject(); }

void UseRepositoryDialog::on_nextButton_clicked() {
  QModelIndexList selected = ui->repoTable->selectionModel()->selectedRows();
  if (selected.isEmpty() || selected.first().row() == 0) {
    MessageBoxDecorator::ShowMessageBox(this, "Repository Not Selected",
                                        "Please select a repository.",
                                        QMessageBox::Warning);
    return;
  }

  int row = selected.first().row();
  std::string created_at = ui->repoTable->item(row, 0)->text().toStdString();
  std::string name = ui->repoTable->item(row, 1)->text().toStdString();
  std::string type =
      ui->repoTable->item(row, 2)->text().toLower().toStdString();
  std::string path = ui->repoTable->item(row, 3)->text().toStdString();

  QString password = ui->passwordInput->text();
  if (Repository::GetHashedPassword(password.toStdString()) !=
      repos[row - 1].password_hash) {
    MessageBoxDecorator::ShowMessageBox(this, "Incorrect Password",
                                        "Please check the repository password.",
                                        QMessageBox::Warning);
    return;
  }

  QEventLoop loop;
  bool result = false;
  ProgressBoxDecorator::RunProgressBox(
      this, "Verifying repository...", [&]() -> bool {
        QThread::sleep(5);
        result = true;
        QMetaObject::invokeMethod(&loop, "quit", Qt::QueuedConnection);
        return true;
      });

  loop.exec();
  if (result) accept();
}
