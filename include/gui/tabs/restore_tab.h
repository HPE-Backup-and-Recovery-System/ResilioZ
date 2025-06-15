#ifndef RESTORE_TAB_H
#define RESTORE_TAB_H

#include <QWidget>

namespace Ui {
class RestoreTab;
}

class RestoreTab : public QWidget {
  Q_OBJECT

 public:
  explicit RestoreTab(QWidget *parent = nullptr);
  ~RestoreTab();

 private:
  Ui::RestoreTab *ui;
};

#endif  // RESTORE_TAB_H
