#ifndef RESTORE_TAB_H
#define RESTORE_TAB_H

#include <QWidget>

#include "gui/decorators/core.h"
#include "repositories/repository.h"
#include "services/repository_service.h"
#include "systems/system.h"

namespace Ui {
class RestoreTab;
}

class RestoreTab : public QWidget {
  Q_OBJECT

 public:
  explicit RestoreTab(QWidget* parent = nullptr);
  ~RestoreTab();

 private slots:
  // Navigation
  void on_nextButton_clicked();
  void on_backButton_clicked();
  void on_backButton_2_clicked();
  void on_nextButton_2_clicked();
  void on_backButton_3_clicked();
  void on_nextButton_3_clicked();

  // Selection of repository through dialog
  void on_chooseRepoButton_clicked();
  void on_chooseRepoButton_2_clicked();

  // Event handler on change of selection of backup file
  void onFileSelected();

  // Enables the input
  void on_customDestination_clicked();

  // Clears and disables the input
  void on_originalDestination_clicked();

  // Menu Buttons
  void on_restoreButton_clicked();
  void on_verifyButton_clicked();

 private:
  Ui::RestoreTab* ui;
  RestoreGUI* restore_;

  // Store repo details
  Repository* repository_;

  // Store backup file details (file) - dependent on repo selection.
  std::string backup_file;

  // Store backup destination details (folder)
  std::string backup_destination;

  // Vectors for File Details
  std::vector<std::string> success_files_;
  std::vector<std::string> corrupt_files_;
  std::vector<std::string> failed_files_;

  // Store summary of restore attempt
  RestoreSummary summary_;

  // Previous Index for Callbacks
  int prev_index_;

  // Handles update of progress bar
  void updateProgress();

  // Handles update of nav buttons
  void updateButtons();

  // Event handler on change of submenu step
  void onAttemptRestorePageChanged(int index);
  void onVerifyBackupPageChanged(int index);

  // Loads files given a partiuclar backup repo
  void loadFileTables();

  // handle resizes and column sizing
  void setColSize(int tableWidth);
  void resizeEvent(QResizeEvent* event);

  // Sets repository_
  bool handleSelectRepo();

  // Sets backup_file
  bool handleSelectFile();

  // Sets backup_destination
  bool handleSelectDestination();

  void restoreBackup();
  void verifyBackup();
};

#endif  // RESTORE_TAB_H
