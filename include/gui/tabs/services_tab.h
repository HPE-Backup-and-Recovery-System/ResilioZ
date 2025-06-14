#ifndef SERVICES_TAB_H
#define SERVICES_TAB_H

#include <QWidget>

namespace Ui {
class ServicesTab;
}

class ServicesTab : public QWidget {
  Q_OBJECT

 public:
  explicit ServicesTab(QWidget *parent = nullptr);
  ~ServicesTab();

 private:
  Ui::ServicesTab *ui;
};

#endif  // SERVICES_TAB_H
