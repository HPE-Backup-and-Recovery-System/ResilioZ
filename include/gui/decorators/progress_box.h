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
  static void runProgressBox(
      QWidget* parent, std::function<bool(std::function<void(int)>)> task,
      const QString& message,
      const QString& success_message = "Operation completed.",
      const QString& failure_message = "Operation failed.",
      std::function<void(bool)> onFinishCallback = [](bool) {});

  // Indeterminate Task (No Progress)
  static void runProgressBox(
      QWidget* parent, std::function<bool()> task, const QString& message,
      const QString& success_message = "Operation completed.",
      const QString& failure_message = "Operation failed.",
      std::function<void(bool)> onFinishCallback = [](bool) {});

 private:
  static QString getStyleSheet();
  static QDialog* createProgressDialog(QWidget* parent, const QString& message,
                                       bool determinate);
};

#endif  // PROGRESS_BOX_H
