#ifndef CREATE_SCHEDULE_DIALOG_H
#define CREATE_SCHEDULE_DIALOG_H

#include <QDialog>

#include "utils/scheduler_request_manager.h"
#include "backup_restore/backup.hpp"
#include "schedulers/schedule.h"

namespace Ui {
class CreateScheduleDialog;
}

class CreateScheduleDialog : public QDialog
{
    Q_OBJECT

public:
    explicit CreateScheduleDialog(QWidget *parent = nullptr);
    ~CreateScheduleDialog();

private slots:
  void on_nextButton_clicked();
  void on_backButton_clicked();

private:
    std::string cron_string;
    std::string source_path;
    std::string destination_path;
    std::string remarks;
    BackupType type;

    Ui::CreateScheduleDialog *ui;
    SchedulerRequestManager *request_mgr;

    void updateProgress();
    void updateButtons();

    bool handleEndpointDetails();
    bool handleBackupDetails();
    bool handleScheduleDetails();

    void addSchedule();
};

#endif // CREATE_SCHEDULE_DIALOG_H
