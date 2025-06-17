#include "gui/decorators/progress_box.h"

#include <QApplication>
#include <QMetaObject>
#include <QProgressBar>
#include <QPushButton>
#include <QThread>
#include <QVBoxLayout>
#include <QtConcurrent>

#include "gui/decorators/message_box.h"

QString ProgressBoxDecorator::GetStyleSheet() {
  return QString(R"(
    QProgressBar {
      border: 1px solid #CCC;
      border-radius: 5px;
      height: 18px;
      text-align: center;
    }
    
    QProgressBar::chunk {
      background-color: rgb(6, 134, 103);
      width: 20px;
    }
    )");
}

QDialog* ProgressBoxDecorator::CreateProgressDialog(QWidget* parent,
                                                    const QString& message,
                                                    bool determinate) {
  QDialog* dialog = new QDialog(parent);
  dialog->setWindowTitle("Please wait");
  dialog->setModal(true);
  dialog->setMinimumSize(360, 120);

  QLabel* label = new QLabel(message, dialog);
  label->setAlignment(Qt::AlignCenter);
  label->setStyleSheet("font-size: 12pt; font-weight: 500;");

  QProgressBar* progressBar = new QProgressBar(dialog);
  progressBar->setObjectName("progressBar");
  progressBar->setStyleSheet(GetStyleSheet());
  progressBar->setTextVisible(true);
  if (!determinate) {
    progressBar->setRange(0, 0);  // Indeterminate (pulsing)
  } else {
    progressBar->setRange(0, 100);
    progressBar->setValue(0);
  }

  QVBoxLayout* layout = new QVBoxLayout(dialog);
  layout->addWidget(label);
  layout->addWidget(progressBar);
  dialog->setLayout(layout);
  dialog->show();

  QApplication::processEvents();
  return dialog;
}

// Determinate
void ProgressBoxDecorator::RunProgressBox(
    QWidget* parent, const QString& message,
    std::function<bool(std::function<void(int)>)> task) {
  auto dialog = CreateProgressDialog(parent, message, true);
  QProgressBar* bar = dialog->findChild<QProgressBar*>("progressBar");

  QtConcurrent::run([=]() {
    bool result = task([=](int value) {
      QMetaObject::invokeMethod(bar, [=]() { bar->setValue(value); });
    });

    QMetaObject::invokeMethod(dialog, [=]() {
      dialog->accept();
      if (result) {
        MessageBoxDecorator::ShowMessageBox(parent, "Success",
                                            "Operation completed.",
                                            QMessageBox::Information);
      } else {
        MessageBoxDecorator::ShowMessageBox(
            parent, "Failed", "Operation failed.", QMessageBox::Critical);
      }
    });
  });
}

// Indeterminate
void ProgressBoxDecorator::RunProgressBox(QWidget* parent,
                                          const QString& message,
                                          std::function<bool()> task) {
  auto dialog = CreateProgressDialog(parent, message, false);

  QtConcurrent::run([=]() {
    bool result = task();

    QMetaObject::invokeMethod(dialog, [=]() {
      dialog->accept();
      if (result) {
        MessageBoxDecorator::ShowMessageBox(parent, "Success",
                                            "Operation completed.",
                                            QMessageBox::Information);
      } else {
        MessageBoxDecorator::ShowMessageBox(
            parent, "Failed", "Operation failed.", QMessageBox::Critical);
      }
    });
  });
}
