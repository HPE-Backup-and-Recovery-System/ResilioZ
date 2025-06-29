#include "gui/dialog/use_repository_dialog.h"

#include <QModelIndexList>
#include <QThread>
#include <QTimer>

#include "gui/decorators/message_box.h"
#include "gui/decorators/progress_box.h"
#include "gui/dialog/ui_use_repository_dialog.h"
#include "repositories/all.h"
#include "utils/utils.h"

UseRepositoryDialog::UseRepositoryDialog(QWidget* parent)
    : QDialog(parent), ui(new Ui::UseRepositoryDialog) {
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
          &UseRepositoryDialog::checkSelection);
}

UseRepositoryDialog::~UseRepositoryDialog() {
  delete ui;
  delete repodata_mgr_;

  repository_ = nullptr;
}

void UseRepositoryDialog::setRepository(Repository* repository) {
  if (repository != nullptr) {
    delete repository_;
    repository_ = nullptr;
  }
  repository_ = repository;
}

Repository* UseRepositoryDialog::getRepository() const { return repository_; }

void UseRepositoryDialog::resizeEvent(QResizeEvent* event) {
  QDialog::resizeEvent(event);  // QWidget::resizeEvent(event); for QWidgets
  setColSize(ui->repoTable->viewport()->width());
}

void UseRepositoryDialog::checkSelection() {
  QModelIndexList selected = ui->repoTable->selectionModel()->selectedRows();

  bool validSelection = !selected.isEmpty() && selected.first().row() != 0;
  ui->nextButton->setEnabled(validSelection);
  ui->passwordInput->setEnabled(validSelection);
}

void UseRepositoryDialog::setColSize(int tableWidth) {
  int col_created_at = 200;
  int col_name = 240;
  int col_type = 100;
  int col_path = tableWidth - col_created_at - col_name - col_type;

  ui->repoTable->setColumnWidth(0, col_created_at);  // Created At
  ui->repoTable->setColumnWidth(1, col_name);        // Name
  ui->repoTable->setColumnWidth(2, col_type);        // Type
  ui->repoTable->setColumnWidth(3, col_path);        // Path
}

void UseRepositoryDialog::fillTable() {
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
    MessageBoxDecorator::showMessageBox(this, "Repository Not Selected",
                                        "Please select a repository.",
                                        QMessageBox::Warning);
    return;
  }

  ui->nextButton->setEnabled(false);
  useRepository(selected.first().row());
}

void UseRepositoryDialog::useRepository(int row) {
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
    setRepository(
        new LocalRepository(path, name, password.toStdString(), created_at));
  } else if (type == "nfs") {
    setRepository(
        new NFSRepository(path, name, password.toStdString(), created_at));
  } else if (type == "remote") {
    setRepository(
        new RemoteRepository(path, name, password.toStdString(), created_at));
  } else {
    MessageBoxDecorator::showMessageBox(
        this, "Failure", "Could not select repository due to invalid type.",
        QMessageBox::Warning);

    Logger::Log("GUI: Repository selection failure due to invalid type.",
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
            Logger::SystemLog("GUI | Selection Failed for Repository: " +
                              repository_->GetRepositoryInfoString() +
                              " as it does not exist");

            setFailureMessage("Repository not found at location: " +
                              QString::fromStdString(repository_->GetPath()));

            return false;
          }

          setWaitMessage("Loading repository...");

          Logger::SystemLog(
              "GUI | Repository: " + repository_->GetName() + " [" +
              Repository::GetFormattedTypeString(repository_->GetType()) +
              "] loaded from location: " + repository_->GetPath());

          setSuccessMessage(
              "Repository: " + QString::fromStdString(repository_->GetName()) +
              " [" +
              QString::fromStdString(
                  Repository::GetFormattedTypeString(repository_->GetType())) +
              "] selected from location: " +
              QString::fromStdString(repository_->GetPath()));
          return true;

        } catch (const std::exception& e) {
          Logger::SystemLog(
              "GUI | Cannot select repository: " + std::string(e.what()),
              LogLevel::ERROR);
          setFailureMessage("Repository selection failed: " +
                            QString::fromStdString(e.what()));
          return false;
        }
      },
      "Loading repository...", "Repository selected successfully.",
      "Repository selection failed.",
      [&](bool success) {
        if (success) {
          accept();
        } else {
          ui->nextButton->setEnabled(true);
        }
      });
}
