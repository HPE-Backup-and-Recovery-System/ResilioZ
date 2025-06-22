#include "gui/dialog/delete_schedule_dialog.h"
#include "gui/dialog/ui_delete_schedule_dialog.h"

DeleteScheduleDialog::DeleteScheduleDialog(QWidget *parent) :
    QDialog(parent),
    ui(new Ui::DeleteScheduleDialog)
{
    ui->setupUi(this);
}

DeleteScheduleDialog::~DeleteScheduleDialog()
{
    delete ui;
}
