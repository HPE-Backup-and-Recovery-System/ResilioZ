#include "gui/dialog/delete_schedule_dialog.h"
#include "gui/dialog/ui_delete_schedule_dialog.h"

#include <QTimer>

#include "gui/decorators/message_box.h"

DeleteScheduleDialog::DeleteScheduleDialog(QWidget *parent) :
    QDialog(parent),
    ui(new Ui::DeleteScheduleDialog)
{
    ui->setupUi(this);

    ui->backButton->setAutoDefault(true);
    ui->backButton->setDefault(false);
    ui->nextButton->setAutoDefault(true);
    ui->nextButton->setDefault(true);

    request_mgr = new SchedulerRequestManager();
    schedules = request_mgr->SendViewRequest();

    fillTable();
    checkSelection();

    auto* header = ui->schedule_table->horizontalHeader();
    header->setSectionResizeMode(QHeaderView::Fixed);
    ui->schedule_table->verticalHeader()->setVisible(false);

    // Polished Features for Table
    ui->schedule_table->setEditTriggers(QAbstractItemView::NoEditTriggers);
    ui->schedule_table->setSelectionBehavior(QAbstractItemView::SelectRows);
    ui->schedule_table->setSelectionMode(QAbstractItemView::SingleSelection);
    ui->schedule_table->setShowGrid(true);
    ui->schedule_table->setFocusPolicy(Qt::NoFocus);

    // Table Size
    QTimer::singleShot(
        0, this, [this]() { setColSize(ui->schedule_table->viewport()->width()); });

    connect(ui->schedule_table->selectionModel(),
            &QItemSelectionModel::selectionChanged, this,
            &DeleteScheduleDialog::checkSelection);
}

DeleteScheduleDialog::~DeleteScheduleDialog()
{
    delete ui;
    delete request_mgr;
}

void DeleteScheduleDialog::resizeEvent(QResizeEvent* event) {
  QDialog::resizeEvent(event);  // QWidget::resizeEvent(event); for QWidgets
  setColSize(ui->schedule_table->viewport()->width());
}

void DeleteScheduleDialog::checkSelection() {
  QModelIndexList selected = ui->schedule_table->selectionModel()->selectedRows();

  bool validSelection = !selected.isEmpty() && selected.first().row() != 0;
  ui->nextButton->setEnabled(validSelection);
}

void DeleteScheduleDialog::setColSize(int tableWidth) {
    int col_name = 100;
    int col_schedule = 200;
    int col_backup_type = 200;
    int col_remarks = 200;
    
    int space_left = tableWidth - col_name - col_schedule - col_backup_type - col_remarks;
    int col_source = space_left / 2;
    int col_destination = space_left / 2;

    ui->schedule_table->setColumnWidth(0, col_name);  
    ui->schedule_table->setColumnWidth(1, col_schedule);
    ui->schedule_table->setColumnWidth(2, col_source);
    ui->schedule_table->setColumnWidth(3, col_destination);
    ui->schedule_table->setColumnWidth(4, col_remarks);
    ui->schedule_table->setColumnWidth(5, col_backup_type);
}

void DeleteScheduleDialog::fillTable(){
    ui->schedule_table->clearContents();
    int schedule_count = static_cast<int>(schedules.size());

    ui->schedule_table->setSpan(0, 0, 1, 6);
    auto* noSelection = new QTableWidgetItem("< No Selection >");
    noSelection->setTextAlignment(Qt::AlignCenter);
    noSelection->setForeground(Qt::darkGray);
    ui->schedule_table->setItem(0, 0, noSelection);

    ui->schedule_table->setRowCount(schedule_count + 1);

    for (int row_ = 0; row_ < schedule_count; row_++) {
        const Schedule& schedule = schedules[row_];
        int row = row_ + 1;

        QString schedule_name = QString::fromStdString(schedule.name);
        QString schedule_schedule = QString::fromStdString(schedule.schedule);
        QString schedule_source = QString::fromStdString(schedule.source);
        QString schedule_destination = QString::fromStdString(schedule.destination);
        QString schedule_remarks = QString::fromStdString(schedule.remarks);
        QString schedule_type = QString::fromStdString(schedule.backup_type);

        auto* nameItem = new QTableWidgetItem(schedule_name);
        nameItem->setTextAlignment(Qt::AlignCenter);
        nameItem->setToolTip(schedule_name);
        ui->schedule_table->setItem(row, 0, nameItem);

        auto* scheduleItem = new QTableWidgetItem(schedule_schedule);
        scheduleItem->setTextAlignment(Qt::AlignCenter);
        scheduleItem->setToolTip(schedule_schedule);
        ui->schedule_table->setItem(row, 1, scheduleItem);

        auto* sourceItem = new QTableWidgetItem(schedule_source);
        sourceItem->setTextAlignment(Qt::AlignCenter);
        sourceItem->setToolTip(schedule_source);
        ui->schedule_table->setItem(row, 2, sourceItem);

        auto* destinationItem = new QTableWidgetItem(schedule_destination);
        destinationItem->setTextAlignment(Qt::AlignCenter);
        destinationItem->setToolTip(schedule_destination);
        ui->schedule_table->setItem(row, 3, destinationItem);

        auto* remarksItem = new QTableWidgetItem(schedule_remarks);
        remarksItem->setTextAlignment(Qt::AlignCenter);
        remarksItem->setToolTip(schedule_remarks);
        ui->schedule_table->setItem(row, 4, remarksItem);

        auto* typeItem = new QTableWidgetItem(schedule_type);
        typeItem->setTextAlignment(Qt::AlignCenter);
        typeItem->setToolTip(schedule_type);
        ui->schedule_table->setItem(row, 5, typeItem);

    }

    ui->schedule_table->selectRow(0);
}

void DeleteScheduleDialog::on_backButton_clicked() { reject(); }

void DeleteScheduleDialog::on_nextButton_clicked() {
    QModelIndexList selected = ui->schedule_table->selectionModel()->selectedRows();
    if (selected.isEmpty() || selected.first().row() == 0) {
        MessageBoxDecorator::showMessageBox(this, "Schedule Not Selected",
                                            "Please select a schedule.",
                                            QMessageBox::Warning);
        return;
    }

    ui->nextButton->setEnabled(false);
    deleteSchedule(selected.first().row());
}

void DeleteScheduleDialog::deleteSchedule(int row) {
    std::string schedule_id = ui->schedule_table->item(row, 0)->text().toStdString();

    // To do progress bar
    request_mgr->SendDeleteRequest(schedule_id);
    accept();
}
