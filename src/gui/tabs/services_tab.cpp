#include "gui/tabs/services_tab.h"

#include "gui/dialog/create_repository_dialog.h"
#include "gui/dialog/delete_repository_dialog.h"
#include "gui/dialog/list_repositories_dialog.h"
#include "gui/tabs/ui_services_tab.h"

ServicesTab::ServicesTab(QWidget *parent)
    : QWidget(parent), ui(new Ui::ServicesTab) {
  ui->setupUi(this);
  ui->stackedWidget->setCurrentIndex(0);
}

ServicesTab::~ServicesTab() { delete ui; }

void ServicesTab::on_repoSvcBtn_clicked() {
  ui->stackedWidget->setCurrentIndex(1);
}

void ServicesTab::on_schSvcBtn_clicked() {
  ui->stackedWidget->setCurrentIndex(2);
}

void ServicesTab::on_backButton_clicked() {
  ui->stackedWidget->setCurrentIndex(0);
}

void ServicesTab::on_backButton_2_clicked() {
  ui->stackedWidget->setCurrentIndex(0);
}

void ServicesTab::on_createRepo_clicked() {
  CreateRepositoryDialog dialog(this);
  dialog.setWindowFlags(Qt::Window);
  dialog.exec();
}

void ServicesTab::on_listRepo_clicked() {
  ListRepositoriesDialog dialog(this);
  dialog.setWindowFlags(Qt::Window);
  dialog.exec();
}

void ServicesTab::on_deleteRepo_clicked() {
  DeleteRepositoryDialog dialog(this);
  dialog.setWindowFlags(Qt::Window);
  dialog.exec();
}
