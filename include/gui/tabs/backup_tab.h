#ifndef BACKUP_TAB_H
#define BACKUP_TAB_H

#include <QWidget>

namespace Ui {
class BackupTab;
}

class BackupTab : public QWidget {
  Q_OBJECT

 public:
  explicit BackupTab(QWidget *parent = nullptr);
  ~BackupTab();

 private:
  Ui::BackupTab *ui;
};

#endif  // BACKUP_TAB_H
