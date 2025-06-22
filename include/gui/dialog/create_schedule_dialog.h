#ifndef CREATE_SCHEDULE_DIALOG_H
#define CREATE_SCHEDULE_DIALOG_H

#include <QDialog>

namespace Ui {
class CreateScheduleDialog;
}

class CreateScheduleDialog : public QDialog
{
    Q_OBJECT

public:
    explicit CreateScheduleDialog(QWidget *parent = nullptr);
    ~CreateScheduleDialog();

private:
    Ui::CreateScheduleDialog *ui;
};

#endif // CREATE_SCHEDULE_DIALOG_H
