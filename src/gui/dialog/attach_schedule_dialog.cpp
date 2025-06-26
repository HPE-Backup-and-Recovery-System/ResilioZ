#include "gui/dialog/attach_schedule_dialog.h"
#include "gui/dialog/ui_attach_schedule_dialog.h"

#include "backup_restore/backup.hpp"
#include "utils/utils.h"

#include "gui/decorators/message_box.h"

#include <iostream>

AttachScheduleDialog::AttachScheduleDialog(QWidget *parent, 
    std::string source_path_, std::string destination_path_, 
    std::string remarks_, BackupType backup_type_) :
    QDialog(parent),
    ui(new Ui::AttachScheduleDialog)
{
    ui->setupUi(this);
    
    source_path = source_path_;
    destination_path = destination_path_;
    remarks = remarks_;
    backup_type = backup_type_;

    fillData();
}

AttachScheduleDialog::~AttachScheduleDialog()
{
    delete ui;
}

std::string AttachScheduleDialog::getSchedule(){
    return cron_string;
}

void AttachScheduleDialog::fillData(){

    this->ui->sourceLabel->setText(QString::fromStdString(source_path));
    this->ui->destLabel->setText(QString::fromStdString(destination_path));
    this->ui->remarksLabel->setText(QString::fromStdString(remarks));
    
    if (backup_type == BackupType::FULL){
        this->ui->typeLabel->setText("Full");
    }
    else if (backup_type == BackupType::DIFFERENTIAL){
        this->ui->typeLabel->setText("Differential");
    }
    else if (backup_type == BackupType::INCREMENTAL){
        this->ui->typeLabel->setText("Incremental");
    }
}

bool AttachScheduleDialog::handleSubmit(){
    std::string cron_string_ = ui->scheduleInput->text().toStdString();
    if (cron_string_ == "" || !Validator::IsValidCronString(cron_string_)){
        MessageBoxDecorator::showMessageBox(
            this, "Invalid Input", "Schedule cron string is invalid.", QMessageBox::Warning);
        
        return false;
    }

    cron_string = cron_string_;
    return true;
}

void AttachScheduleDialog::on_backButton_clicked()
{
    reject();
}


void AttachScheduleDialog::on_nextButton_clicked()
{
    if (handleSubmit()){
        accept();
        return;
    }
    reject();
}

