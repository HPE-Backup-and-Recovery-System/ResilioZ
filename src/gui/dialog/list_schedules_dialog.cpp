#include "gui/dialog/list_schedules_dialog.h"

#include <QTimer>

#include "gui/dialog/ui_list_schedules_dialog.h"
#include "schedulers/schedule.h"
#include "utils/scheduler_request_manager.h"

ListSchedulesDialog::ListSchedulesDialog(QWidget* parent)
    : QDialog(parent), ui(new Ui::ListSchedulesDialog) {
  ui->setupUi(this);
  request_mgr = new SchedulerRequestManager();
  schedules = request_mgr->SendViewRequest();

  fillTable();
  auto* header = ui->schedule_table->horizontalHeader();
  header->setSectionResizeMode(QHeaderView::Fixed);
  ui->schedule_table->verticalHeader()->setVisible(false);

  // Polished Features for Table
  ui->schedule_table->setSelectionMode(QAbstractItemView::NoSelection);
  ui->schedule_table->setEditTriggers(QAbstractItemView::NoEditTriggers);
  ui->schedule_table->setShowGrid(true);
  ui->schedule_table->setFocusPolicy(Qt::NoFocus);

  // Table Size
  QTimer::singleShot(0, this, [this]() {
    setColSize(ui->schedule_table->viewport()->width());
  });
}

ListSchedulesDialog::~ListSchedulesDialog() {
  delete ui;
  delete request_mgr;
}

void ListSchedulesDialog::resizeEvent(QResizeEvent* event) {
  QDialog::resizeEvent(event);
  setColSize(ui->schedule_table->viewport()->width());
}

void ListSchedulesDialog::setColSize(int tableWidth) {
  int col_name = 90;
  int col_schedule = 125;
  int col_backup_type = 180;
  int col_remarks = 200;

  int space_left =
      tableWidth - col_name - col_schedule - col_backup_type - col_remarks;
  int col_source = space_left / 2;
  int col_destination = space_left / 2;

  ui->schedule_table->setColumnWidth(0, col_name);
  ui->schedule_table->setColumnWidth(1, col_schedule);
  ui->schedule_table->setColumnWidth(2, col_source);
  ui->schedule_table->setColumnWidth(3, col_destination);
  ui->schedule_table->setColumnWidth(4, col_remarks);
  ui->schedule_table->setColumnWidth(5, col_backup_type);
}

void ListSchedulesDialog::fillTable() {
  ui->schedule_table->clearContents();
  int schedule_count = static_cast<int>(schedules.size());

  if (!schedule_count) {
    ui->schedule_table->setRowCount(1);
    ui->schedule_table->setSpan(0, 0, 1, 6);
    auto* noRepo = new QTableWidgetItem("< No Schedules Created >");
    noRepo->setTextAlignment(Qt::AlignCenter);
    noRepo->setForeground(Qt::darkGray);
    ui->schedule_table->setItem(0, 0, noRepo);
    return;
  }
  ui->schedule_table->setRowCount(schedule_count);

  for (int row = 0; row < schedule_count; row++) {
    const Schedule& schedule = schedules[row];

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
}

void ListSchedulesDialog::on_backButton_clicked() { accept(); }