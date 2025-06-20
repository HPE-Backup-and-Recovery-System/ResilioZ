#include "gui/decorators/message_box.h"

#include <QMessageBox>
#include <QPushButton>
#include <QString>
#include <QWidget>

QString MessageBoxDecorator::getStyleSheet() {
  return QString(R"(
QMessageBox {
    font-size: 12pt;
}

QLabel {
    font-weight: 500;
}

QPushButton {
    background-color:rgb(6, 134, 103);
    color: white;
    font-size: 12pt;
    padding: 0px 16px;
    text-align: center;
}
    )");
}

void MessageBoxDecorator::showMessageBox(QWidget* parent, const QString& title,
                                         const QString& text,
                                         QMessageBox::Icon icon) {
  QMessageBox box(parent);
  box.setWindowTitle(title);
  box.setText(text);
  box.setIcon(icon);
  box.setStyleSheet(getStyleSheet());
  QPushButton* ok = box.addButton(QMessageBox::Ok);
  ok->setAutoDefault(true);
  ok->setDefault(true);
  ok->setIcon(QIcon());
  box.setMinimumSize(QSize(480, 240));  // Does not work but Okay...
  box.exec();
}
