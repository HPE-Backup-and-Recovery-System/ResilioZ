#include "gui/decorators/progress_box.h"

#include <QApplication>
#include <QCloseEvent>
#include <QMetaObject>
#include <QProgressBar>
#include <QThread>
#include <QVBoxLayout>
#include <QtConcurrent>

#include "gui/decorators/message_box.h"

QString ProgressBoxDecorator::getStyleSheet() {
  return QString(R"(
    QProgressBar {
      border: 1px solid rgb(29, 31, 39);
      height: 20px;
      text-align: center;
      font-weight: bold;
    }
    
    QProgressBar::chunk {
      background-color: rgb(6, 134, 103);
      width: 32px;
    }
    )");
}

QDialog* ProgressBoxDecorator::createProgressDialog(QWidget* parent,
                                                    QLabel*& label,
                                                    const QString& message,
                                                    bool determinate) {
  class BlockingDialog : public QDialog {
   public:
    using QDialog::QDialog;

   protected:
    void closeEvent(QCloseEvent* event) override { event->ignore(); }
  };

  QDialog* dialog = new BlockingDialog(parent);
  dialog->setWindowTitle("Please Wait");
  dialog->setModal(true);
  dialog->setMinimumSize(360, 160);
  dialog->setWindowFlags(dialog->windowFlags() & ~Qt::WindowCloseButtonHint);
  dialog->setWindowFlag(Qt::WindowContextHelpButtonHint, false);

  label = new QLabel(message, dialog);
  label->setWordWrap(true);
  label->setAlignment(Qt::AlignLeft);
  label->setStyleSheet(
      "font-size: 12pt; font-weight: 500; color: rgb(29, 31, 39)");

  QProgressBar* progressBar = new QProgressBar(dialog);
  progressBar->setObjectName("progressBar");
  progressBar->setStyleSheet(getStyleSheet());
  progressBar->setTextVisible(false);
  if (!determinate) {
    progressBar->setRange(0, 0);
  } else {
    progressBar->setRange(0, 100);
    progressBar->setValue(0);
  }

  QVBoxLayout* layout = new QVBoxLayout(dialog);
  layout->addWidget(label, 1);
  layout->addStretch();
  layout->addWidget(progressBar, 0);
  dialog->setLayout(layout);
  dialog->show();

  QApplication::processEvents();
  return dialog;
}

void ProgressBoxDecorator::runProgressBoxDeterminate(
    QWidget* parent,
    std::function<bool(std::function<void(int)> setProgress,
                       std::function<void(const QString&)> setWaitMessage,
                       std::function<void(const QString&)> setSuccessMessage,
                       std::function<void(const QString&)> setFailureMessage)>
        task,
    QString wait_message, QString success_message, QString failure_message,
    std::function<void(bool)> onFinishCallback) {
  QLabel* label = nullptr;
  QDialog* dialog = createProgressDialog(parent, label, wait_message, true);
  QProgressBar* bar = dialog->findChild<QProgressBar*>("progressBar");

  QtConcurrent::run([=]() mutable {
    bool result = false;

    auto setProgress = [&](int value) {
      QMetaObject::invokeMethod(bar, [=]() { bar->setValue(value); });
    };
    auto setWaitMsg = [&](const QString& msg) {
      QMetaObject::invokeMethod(label, [=]() { label->setText(msg); });
    };
    auto setSuccessMsg = [&](const QString& msg) { success_message = msg; };
    auto setFailureMsg = [&](const QString& msg) { failure_message = msg; };

    result = task(setProgress, setWaitMsg, setSuccessMsg, setFailureMsg);

    QMetaObject::invokeMethod(dialog, [=]() {
      dialog->accept();
      if (result) {
        MessageBoxDecorator::showMessageBox(parent, "Success", success_message,
                                            QMessageBox::Information);
      } else {
        MessageBoxDecorator::showMessageBox(parent, "Failure", failure_message,
                                            QMessageBox::Critical);
      }
      onFinishCallback(result);
    });
  });
}

void ProgressBoxDecorator::runProgressBoxIndeterminate(
    QWidget* parent,
    std::function<bool(std::function<void(const QString&)> setWaitMessage,
                       std::function<void(const QString&)> setSuccessMessage,
                       std::function<void(const QString&)> setFailureMessage)>
        task,
    QString wait_message, QString success_message, QString failure_message,
    std::function<void(bool)> onFinishCallback) {
  QLabel* label = nullptr;
  QDialog* dialog = createProgressDialog(parent, label, wait_message, false);
  QProgressBar* bar = dialog->findChild<QProgressBar*>("progressBar");

  QtConcurrent::run([=]() mutable {
    bool result = false;

    auto setProgress = [&](int value) {
      QMetaObject::invokeMethod(bar, [=]() { bar->setValue(value); });
    };
    auto setWaitMsg = [&](const QString& msg) {
      QMetaObject::invokeMethod(label, [=]() { label->setText(msg); });
    };
    auto setSuccessMsg = [&](const QString& msg) { success_message = msg; };
    auto setFailureMsg = [&](const QString& msg) { failure_message = msg; };

    result = task(setWaitMsg, setSuccessMsg, setFailureMsg);

    QMetaObject::invokeMethod(dialog, [=]() {
      dialog->accept();
      if (result) {
        MessageBoxDecorator::showMessageBox(parent, "Success", success_message,
                                            QMessageBox::Information);
      } else {
        MessageBoxDecorator::showMessageBox(parent, "Failure", failure_message,
                                            QMessageBox::Critical);
      }
      onFinishCallback(result);
    });
  });
}
