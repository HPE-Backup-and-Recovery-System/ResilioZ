#ifndef LIST_SCHEDULES_DIALOG_H
#define LIST_SCHEDULES_DIALOG_H

#include <QDialog>
#include "utils/scheduler_request_manager.h"
#include "schedulers/schedule.h"

namespace Ui {
class ListSchedulesDialog;
}

class ListSchedulesDialog : public QDialog {
    Q_OBJECT

    public:
        explicit ListSchedulesDialog(QWidget *parent = nullptr);
        ~ListSchedulesDialog();

    private slots:
        void on_backButton_clicked();

    private:
        Ui::ListSchedulesDialog *ui;
        SchedulerRequestManager *request_mgr;
        std::vector<Schedule> schedules;

        void resizeEvent(QResizeEvent *event) override;
        void setColSize(int tableWidth);
        void fillTable();
};

#endif // LIST_SCHEDULES_DIALOG_H
