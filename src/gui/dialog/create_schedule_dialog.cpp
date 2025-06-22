#include "gui/dialog/create_schedule_dialog.h"
#include "gui/dialog/ui_create_schedule_dialog.h"

CreateScheduleDialog::CreateScheduleDialog(QWidget *parent) :
    QDialog(parent),
    ui(new Ui::CreateScheduleDialog)
{
    ui->setupUi(this);
}

CreateScheduleDialog::~CreateScheduleDialog()
{
    delete ui;
}
