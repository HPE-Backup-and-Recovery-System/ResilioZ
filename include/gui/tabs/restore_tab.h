#ifndef RESTORE_TAB_H
#define RESTORE_TAB_H

#include <QWidget>
#include "repositories/repository.h"
#include "services/all.h"
#include "systems/system.h"

namespace Ui {
class RestoreTab;
}

class RestoreTab : public QWidget {
  Q_OBJECT

 public:
  explicit RestoreTab(QWidget *parent = nullptr);
  ~RestoreTab();

private slots:
    // Restore menu options
    void on_restoreButton_clicked(); // Normal restore menu
    void on_retryButton_clicked(); // Retry failed restore menu

    // Navigation inside submenu
    void on_nextButton_clicked();
    void on_backButton_clicked();

    // Selection of repository
    void on_chooseRepoButton_clicked();
    
    // Event handler on change of selection of backup file
    void onFileSelected();

    // Event handler on choosing restore destination
    void on_chooseDestination_clicked();

    
    
    private:
    Ui::RestoreTab *ui;
    
    // Store repo details
    Repository* repository_;
    RepositoryService* repo_service_;
    
    // Store backup file details (file) - dependent on repo selection.
    std::string backup_file;
    
    // Store backup destination details (folder)
    std::string backup_destination;
    
    // Handles update of progress bar : Make sure works on all submenus not just plain restore.
    void updateProgress();
    
    // Handles update of nav buttons (state and internal text)
    void updateButtons();
    
    // Event handler on change of submenu step
    void onAttemptRestorePageChanged(int index);
    
    // Loads files given a partiuclar backup repo
    void loadFileTable();
    
    bool handleSelectRepo();
    bool handleSelectFile();
    bool handleSelectDestination();
  };
  
#endif  // RESTORE_TAB_H
