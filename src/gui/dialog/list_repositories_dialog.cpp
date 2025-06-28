#include "gui/dialog/list_repositories_dialog.h"

#include <QModelIndexList>
#include <QThread>
#include <QTimer>

#include "gui/decorators/message_box.h"
#include "gui/decorators/progress_box.h"
#include "gui/dialog/ui_list_repositories_dialog.h"
#include "repositories/repository.h"
#include "utils/utils.h"

ListRepositoriesDialog::ListRepositoriesDialog(QWidget* parent)
    : QDialog(parent), ui(new Ui::ListRepositoriesDialog) {
  ui->setupUi(this);

  repodata_mgr_ = new RepodataManager();
  repos = repodata_mgr_->GetAll();
  fillTable();

  auto* header = ui->repoTable->horizontalHeader();
  header->setSectionResizeMode(QHeaderView::Fixed);
  ui->repoTable->verticalHeader()->setVisible(false);

  // Polished Features for Table
  ui->repoTable->setSelectionMode(QAbstractItemView::NoSelection);
  ui->repoTable->setEditTriggers(QAbstractItemView::NoEditTriggers);
  ui->repoTable->setShowGrid(true);
  ui->repoTable->setFocusPolicy(Qt::NoFocus);

  // Table Size
  QTimer::singleShot(
      0, this, [this]() { setColSize(ui->repoTable->viewport()->width()); });
}

ListRepositoriesDialog::~ListRepositoriesDialog() {
  delete ui;
  delete repodata_mgr_;
}

void ListRepositoriesDialog::resizeEvent(QResizeEvent* event) {
  QDialog::resizeEvent(event);  // QWidget::resizeEvent(event); for QWidgets
  setColSize(ui->repoTable->viewport()->width());
}

void ListRepositoriesDialog::setColSize(int tableWidth) {
  int col_created_at = 200;
  int col_name = 240;
  int col_type = 100;
  int col_path = tableWidth - col_created_at - col_name - col_type;

  ui->repoTable->setColumnWidth(0, col_created_at);  // Created At
  ui->repoTable->setColumnWidth(1, col_name);        // Name
  ui->repoTable->setColumnWidth(2, col_type);        // Type
  ui->repoTable->setColumnWidth(3, col_path);        // Path
}

void ListRepositoriesDialog::fillTable() {
  ui->repoTable->clearContents();
  int repo_count = static_cast<int>(repos.size());

  if (!repo_count) {
    ui->repoTable->setRowCount(1);
    ui->repoTable->setSpan(0, 0, 1, 4);
    auto* noRepo = new QTableWidgetItem("< No Repositories Available >");
    noRepo->setTextAlignment(Qt::AlignCenter);
    noRepo->setForeground(Qt::darkGray);
    ui->repoTable->setItem(0, 0, noRepo);
    return;
  } else {
    ui->repoTable->setSpan(0, 0, 1, 1);
  }

  ui->repoTable->setRowCount(repo_count);
  for (int row = 0; row < repo_count; ++row) {
    const RepoEntry& repo = repos[row];

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
}

void ListRepositoriesDialog::on_backButton_clicked() { accept(); }
