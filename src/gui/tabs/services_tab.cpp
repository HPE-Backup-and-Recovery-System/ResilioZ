#include "gui/tabs/services_tab.h"

#include "gui/tabs/ui_services_tab.h"

ServicesTab::ServicesTab(QWidget *parent)
    : QWidget(parent), ui(new Ui::ServicesTab) {
  ui->setupUi(this);
}

ServicesTab::~ServicesTab() { delete ui; }
