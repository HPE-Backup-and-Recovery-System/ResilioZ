#include "gui/tabs/restore_tab.h"

#include "gui/tabs/ui_restore_tab.h"

RestoreTab::RestoreTab(QWidget *parent)
    : QWidget(parent), ui(new Ui::RestoreTab) {
  ui->setupUi(this);
}

RestoreTab::~RestoreTab() { delete ui; }
