#include "gui/tabs/backup_tab.h"

#include "gui/tabs/ui_backup_tab.h"

BackupTab::BackupTab(QWidget *parent) : QWidget(parent), ui(new Ui::BackupTab) {
  ui->setupUi(this);
}

BackupTab::~BackupTab() { delete ui; }
