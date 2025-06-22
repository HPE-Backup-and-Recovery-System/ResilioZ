#ifndef DELETE_SCHEDULE_DIALOG_H
#define DELETE_SCHEDULE_DIALOG_H

#include <QDialog>

namespace Ui {
class DeleteScheduleDialog;
}

class DeleteScheduleDialog : public QDialog
{
    Q_OBJECT

public:
    explicit DeleteScheduleDialog(QWidget *parent = nullptr);
    ~DeleteScheduleDialog();

private:
    Ui::DeleteScheduleDialog *ui;
};

#endif // DELETE_SCHEDULE_DIALOG_H
