#include "gui/main_window.h"

#include "gui/ui_main_window.h"

MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent), ui(new Ui::MainWindow) {
  ui->setupUi(this);

  ui->tabWidget->setCurrentIndex(0);
}

MainWindow::~MainWindow() { delete ui; }
