#ifndef MESSAGE_BOX_H
#define MESSAGE_BOX_H

#include <QMessageBox>
#include <QString>
#include <QWidget>

class MessageBoxDecorator {
 public:
  static void showMessageBox(QWidget* parent, const QString& title,
                             const QString& text, QMessageBox::Icon icon);

 private:
  static QString getStyleSheet();
};

#endif  // MESSAGE_BOX_H