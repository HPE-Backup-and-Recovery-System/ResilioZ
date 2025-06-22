#include "gui/dialog/list_schedules_dialog.h"

#include "gui/dialog/ui_list_schedules_dialog.h"

ListSchedulesDialog::ListSchedulesDialog(QWidget *parent)
    : QDialog(parent),ui(new Ui::ListSchedulesDialog){
        
    ui->setupUi(this);
}

ListSchedulesDialog::~ListSchedulesDialog()
{
    delete ui;
}
