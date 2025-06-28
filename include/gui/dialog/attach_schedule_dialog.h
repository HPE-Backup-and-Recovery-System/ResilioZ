#ifndef ATTACH_SCHEDULE_DIALOG_H
#define ATTACH_SCHEDULE_DIALOG_H

#include <QDialog>
#include "backup_restore/backup.hpp"

namespace Ui {
class AttachScheduleDialog;
}

class AttachScheduleDialog : public QDialog
{
    Q_OBJECT

public:
    explicit AttachScheduleDialog(QWidget *parent = nullptr, 
        std::string source_path_ = "", std::string destination_path_ = "", 
        std::string remarks_ = "", BackupType backup_type_ = BackupType::FULL);
        
    ~AttachScheduleDialog();
    
    std::string getSchedule();
        
private slots:
    void on_backButton_clicked();

    void on_nextButton_clicked();

private:
    Ui::AttachScheduleDialog *ui;

    std::string cron_string;

    std::string source_path;
    std::string destination_path;
    std::string remarks;
    BackupType backup_type;

    void fillData();
    
    bool handleSubmit();
};

#endif // ATTACH_SCHEDULE_DIALOG_H
