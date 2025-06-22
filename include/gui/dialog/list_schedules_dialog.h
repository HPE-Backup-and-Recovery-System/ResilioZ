#ifndef LIST_SCHEDULES_DIALOG_H
#define LIST_SCHEDULES_DIALOG_H

#include <QDialog>

namespace Ui {
class ListSchedulesDialog;
}

class ListSchedulesDialog : public QDialog {
    Q_OBJECT

    public:
        explicit ListSchedulesDialog(QWidget *parent = nullptr);
        ~ListSchedulesDialog();

private:
    Ui::ListSchedulesDialog *ui;
};

#endif // LIST_SCHEDULES_DIALOG_H
