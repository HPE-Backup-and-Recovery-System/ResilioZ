#include "gui/dialog/delete_repository_dialog.h"

#include <QModelIndexList>
#include <QThread>
#include <QTimer>

#include "gui/decorators/message_box.h"
#include "gui/decorators/progress_box.h"
#include "gui/dialog/ui_delete_repository_dialog.h"
#include "repositories/all.h"
#include "utils/utils.h"

DeleteRepositoryDialog::DeleteRepositoryDialog(QWidget* parent)
    : QDialog(parent), ui(new Ui::DeleteRepositoryDialog) {
  ui->setupUi(this);

  ui->backButton->setAutoDefault(true);
  ui->backButton->setDefault(false);
  ui->nextButton->setAutoDefault(true);
  ui->nextButton->setDefault(true);

  repository_ = nullptr;
  repodata_mgr_ = new RepodataManager();
  repos = repodata_mgr_->GetAll();
  fillTable();
  checkSelection();

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
      0, this, [this]() { setColSize(ui->repoTable->viewport()->width()); });

  connect(ui->repoTable->selectionModel(),
          &QItemSelectionModel::selectionChanged, this,
          &DeleteRepositoryDialog::checkSelection);
}

DeleteRepositoryDialog::~DeleteRepositoryDialog() {
  delete ui;
  delete repodata_mgr_;
  if (repository_) delete repository_;
}

void DeleteRepositoryDialog::resizeEvent(QResizeEvent* event) {
  QDialog::resizeEvent(event);  // QWidget::resizeEvent(event); for QWidgets
  setColSize(ui->repoTable->viewport()->width());
}

void DeleteRepositoryDialog::checkSelection() {
  QModelIndexList selected = ui->repoTable->selectionModel()->selectedRows();

  bool validSelection = !selected.isEmpty() && selected.first().row() != 0;
  ui->nextButton->setEnabled(validSelection);
  ui->passwordInput->setEnabled(validSelection);
}

void DeleteRepositoryDialog::setColSize(int tableWidth) {
  int col_created_at = 200;
  int col_name = 240;
  int col_type = 100;
  int col_path = tableWidth - col_created_at - col_name - col_type;

  ui->repoTable->setColumnWidth(0, col_created_at);  // Created At
  ui->repoTable->setColumnWidth(1, col_name);        // Name
  ui->repoTable->setColumnWidth(2, col_type);        // Type
  ui->repoTable->setColumnWidth(3, col_path);        // Path
}

void DeleteRepositoryDialog::fillTable() {
  ui->repoTable->clearContents();
  ui->repoTable->setRowCount(static_cast<int>(repos.size()) + 1);

  ui->repoTable->setSpan(0, 0, 1, 4);
  auto* noSelection = new QTableWidgetItem("< No Selection >");
  noSelection->setTextAlignment(Qt::AlignCenter);
  noSelection->setForeground(Qt::darkGray);
  ui->repoTable->setItem(0, 0, noSelection);

  for (int i = 0; i < static_cast<int>(repos.size()); ++i) {
    const RepoEntry& repository_ = repos[i];
    int row = i + 1;

    auto* createdItem =
        new QTableWidgetItem(QString::fromStdString(repository_.created_at));
    createdItem->setTextAlignment(Qt::AlignCenter);
    ui->repoTable->setItem(row, 0, createdItem);

    ui->repoTable->setItem(
        row, 1, new QTableWidgetItem(QString::fromStdString(repository_.name)));

    auto* typeItem = new QTableWidgetItem(
        QString::fromStdString(repository_.type).toUpper());
    typeItem->setTextAlignment(Qt::AlignCenter);
    ui->repoTable->setItem(row, 2, typeItem);

    ui->repoTable->setItem(
        row, 3, new QTableWidgetItem(QString::fromStdString(repository_.path)));
  }

  ui->repoTable->selectRow(0);
}

void DeleteRepositoryDialog::on_backButton_clicked() { reject(); }

void DeleteRepositoryDialog::on_nextButton_clicked() {
  QModelIndexList selected = ui->repoTable->selectionModel()->selectedRows();
  if (selected.isEmpty() || selected.first().row() == 0) {
    MessageBoxDecorator::showMessageBox(this, "Repository Not Selected",
                                        "Please select a repository.",
                                        QMessageBox::Warning);
    return;
  }

  ui->nextButton->setEnabled(false);
  deleteRepository(selected.first().row());
}

void DeleteRepositoryDialog::deleteRepository(int row) {
  std::string created_at = ui->repoTable->item(row, 0)->text().toStdString();
  std::string name = ui->repoTable->item(row, 1)->text().toStdString();
  std::string type =
      ui->repoTable->item(row, 2)->text().toLower().toStdString();
  std::string path = ui->repoTable->item(row, 3)->text().toStdString();

  QString password = ui->passwordInput->text();
  if (Repository::GetHashedPassword(password.toStdString()) !=
      repos[row - 1].password_hash) {
    MessageBoxDecorator::showMessageBox(this, "Incorrect Password",
                                        "Please check the repository password.",
                                        QMessageBox::Warning);
    return;
  }

  if (type == "local") {
    repository_ =
        new LocalRepository(path, name, password.toStdString(), created_at);
  } else if (type == "nfs") {
    repository_ =
        new NFSRepository(path, name, password.toStdString(), created_at);
  } else if (type == "remote") {
    repository_ =
        new RemoteRepository(path, name, password.toStdString(), created_at);
  } else {
    MessageBoxDecorator::showMessageBox(
        this, "Failure", "Could not delete repository due to invalid type.",
        QMessageBox::Warning);

    Logger::Log("GUI: Repository deletion failure due to invalid type.",
                LogLevel::ERROR);
    return;
  }

  ProgressBoxDecorator::runProgressBoxIndeterminate(
      this,
      [&](std::function<void(const QString&)> setWaitMessage,
          std::function<void(const QString&)> setSuccessMessage,
          std::function<void(const QString&)> setFailureMessage) -> bool {
        try {
          setWaitMessage("Checking if repository exists...");

          if (!repository_->Exists()) {
            repodata_mgr_->DeleteEntry(repository_->GetName(),
                                       repository_->GetPath());

            Logger::SystemLog("GUI | Deleted entry for repository: " +
                              repository_->GetRepositoryInfoString() +
                              " as it does not exist");

            setFailureMessage("Repository not found at location: " +
                              QString::fromStdString(repository_->GetPath()) +
                              "\nRepository entry has been removed.");
            return false;
          }

          setWaitMessage("Deleting repository...");
          repository_->Delete();

          Logger::SystemLog(
              "GUI | Repository: " + repository_->GetName() + " [" +
              Repository::GetFormattedTypeString(repository_->GetType()) +
              "] deleted at location: " + repository_->GetPath());

          setSuccessMessage(
              "Repository: " + QString::fromStdString(repository_->GetName()) +
              " [" +
              QString::fromStdString(
                  Repository::GetFormattedTypeString(repository_->GetType())) +
              "] deleted from location: " +
              QString::fromStdString(repository_->GetPath()));

          return true;

        } catch (const std::exception& e) {
          Logger::SystemLog(
              "GUI | Cannot delete repository: " + std::string(e.what()),
              LogLevel::ERROR);

          setFailureMessage("Repository deletion failed: " +
                            QString::fromStdString(e.what()));
          return false;
        }
      },
      "Deleting repository...", "Repository deleted successfully.",
      "Repository deletion failed.",
      [&](bool success) {
        if (success) {
          repodata_mgr_->DeleteEntry(repository_->GetName(),
                                     repository_->GetPath());
          accept();
        } else {
          ui->nextButton->setEnabled(true);
        }
      });
}
