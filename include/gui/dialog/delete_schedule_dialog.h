#ifndef DELETE_SCHEDULE_DIALOG_H
#define DELETE_SCHEDULE_DIALOG_H

#include <QDialog>

#include "utils/scheduler_request_manager.h"
#include "schedulers/schedule.h"

namespace Ui {
class DeleteScheduleDialog;
}

class DeleteScheduleDialog : public QDialog
{
    Q_OBJECT

public:
    explicit DeleteScheduleDialog(QWidget *parent = nullptr);
    ~DeleteScheduleDialog();

private slots:
    void on_backButton_clicked();
    void on_nextButton_clicked();

private:
    Ui::DeleteScheduleDialog *ui;
    SchedulerRequestManager *request_mgr;
    std::vector<Schedule> schedules;

    void resizeEvent(QResizeEvent *event) override;
    void checkSelection();

    void setColSize(int tableWidth);
    void fillTable();

    void deleteSchedule(int row);
};

#endif // DELETE_SCHEDULE_DIALOG_H
