#ifndef PROGRESS_BOX_H
#define PROGRESS_BOX_H

#include <QDialog>
#include <QLabel>
#include <QProgressBar>
#include <QString>
#include <QWidget>
#include <functional>

class ProgressBoxDecorator {
 public:
  // Determinate Task (with Progress Callback)
  static void RunProgressBox(
      QWidget* parent, const QString& message,
      std::function<bool(std::function<void(int)>)> task);

  // Indeterminate Task (No Progress)
  static void RunProgressBox(QWidget* parent, const QString& message,
                             std::function<bool()> task);

 private:
  static QString GetStyleSheet();
  static QDialog* CreateProgressDialog(QWidget* parent, const QString& message,
                                       bool determinate);
};

#endif  // PROGRESS_BOX_H
