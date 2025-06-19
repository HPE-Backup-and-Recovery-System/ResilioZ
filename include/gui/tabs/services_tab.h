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

 private slots:
  void on_repoSvcBtn_clicked();
  void on_schSvcBtn_clicked();

  void on_backButton_clicked();
  void on_backButton_2_clicked();

  void on_createRepo_clicked();
  void on_listRepo_clicked();
  void on_deleteRepo_clicked();

  private:
  Ui::ServicesTab *ui;
};

#endif  // SERVICES_TAB_H
