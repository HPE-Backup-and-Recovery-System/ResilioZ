#include "gui/tabs/services_tab.h"

#include "gui/dialog/create_repository_dialog.h"
#include "gui/dialog/create_schedule_dialog.h"
#include "gui/dialog/delete_repository_dialog.h"
#include "gui/dialog/delete_schedule_dialog.h"
#include "gui/dialog/list_repositories_dialog.h"
#include "gui/dialog/list_schedules_dialog.h"
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
  if (dialog.exec() == QDialog::Accepted) {
    delete dialog.getRepository();
  }
}

void ServicesTab::on_listRepo_clicked() {
  ListRepositoriesDialog dialog(this);
  dialog.exec();
}

void ServicesTab::on_deleteRepo_clicked() {
  DeleteRepositoryDialog dialog(this);
  dialog.exec();
}

void ServicesTab::on_listSch_clicked() {
  ListSchedulesDialog dialog(this);
  dialog.exec();
}

void ServicesTab::on_createSch_clicked() {
  CreateScheduleDialog dialog(this);
  dialog.exec();
}

void ServicesTab::on_deleteSch_clicked() {
  DeleteScheduleDialog dialog(this);
  dialog.exec();
}
