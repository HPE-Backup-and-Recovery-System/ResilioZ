#ifndef BACKUP_VERIFICATION_DIALOG_H
#define BACKUP_VERIFICATION_DIALOG_H

#include <QDialog>
#include <string>
#include <vector>

namespace Ui {
class BackupVerificationDialog;
}

class BackupVerificationDialog : public QDialog {
  Q_OBJECT

 public:
  explicit BackupVerificationDialog(
      QWidget* parent = nullptr,
      const std::vector<std::string>& success_files = {},
      const std::vector<std::string>& corrupt_files = {},
      const std::vector<std::string>& fail_files = {});
  ~BackupVerificationDialog();

 private slots:
  void on_backButton_clicked();

 private:
  Ui::BackupVerificationDialog* ui;
  std::vector<std::string> success_files_;
  std::vector<std::string> corrupt_files_;
  std::vector<std::string> fail_files_;

  void fillTables();
};

#endif  // BACKUP_VERIFICATION_DIALOG_H
