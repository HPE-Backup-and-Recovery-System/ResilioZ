#include <iostream>
#include "services/backup_service.h"

int main() {
  std::cout << "===== Backup & Restore CLI System =====\n\n";

  // Create and run the BackupService
  services::BackupService backup_service;
  backup_service.Run();

  std::cout << "\nProgram terminated.\n";
  return 0;
}
