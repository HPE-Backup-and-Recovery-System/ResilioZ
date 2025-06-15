#include "init/gui.h"

#include <QApplication>

#include "gui/main_window.h"

int RunGUI(int argc, char *argv[]) {
  QApplication a(argc, argv);
  MainWindow w;
  w.show();
  return a.exec();
}
