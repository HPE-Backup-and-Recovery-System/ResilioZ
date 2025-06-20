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
  static void runProgressBoxDeterminate(
      QWidget* parent,
      std::function<bool(std::function<void(int)> setProgress,
                         std::function<void(const QString&)> setWaitMessage,
                         std::function<void(const QString&)> setSuccessMessage,
                         std::function<void(const QString&)> setFailureMessage)>
          task,
      QString wait_message = "Please wait...",
      QString success_message = "Operation completed.",
      QString failure_message = "Operation failed.",
      std::function<void(bool)> onFinishCallback = [](bool) {});

  static void runProgressBoxIndeterminate(
      QWidget* parent,
      std::function<bool(std::function<void(const QString&)> setWaitMessage,
                         std::function<void(const QString&)> setSuccessMessage,
                         std::function<void(const QString&)> setFailureMessage)>
          task,
      QString wait_message = "Please wait...",
      QString success_message = "Operation completed.",
      QString failure_message = "Operation failed.",
      std::function<void(bool)> onFinishCallback = [](bool) {});

 private:
  static QString getStyleSheet();
  static QDialog* createProgressDialog(QWidget* parent, QLabel*& label,
                                       const QString& message,
                                       bool determinate);
};

#endif  // PROGRESS_BOX_H
