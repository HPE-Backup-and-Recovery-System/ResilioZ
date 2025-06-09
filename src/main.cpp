#include <exception>
#include <iostream>
#include <vector>

#include "services/repository_service.h"
#include "services/scheduler_service.h"
#include "utils/error_util.h"
#include "utils/logger.h"
#include "utils/user_io.h"

int main(int argc, char** argv) {
  // UserIO::ClearTerminal();
  Logger::TerminalLog("HPE - Backup and Recovery System in Linux...");

  std::vector<std::string> main_menu = {"EXIT...", "Repository Service",
                                        "Schedule Service"};
  while (true) {
    int choice = UserIO::HandleMenuWithSelect(
        UserIO::DisplayMaxTitle("SYSTEM SERVICES", false), main_menu);

    switch (choice) {
      case 0: {
        std::cout << "\n - Exiting...\n\n";
        return EXIT_SUCCESS;
      }
      case 1: {
        RepositoryService service;
        service.Run();
        break;
      }
      case 2: {
        SchedulerService service;
        service.Run();
        break;
      }
      default: {
        Logger::TerminalLog("Menu Mismatch...", LogLevel::ERROR);
      }
    }
  }
  return EXIT_FAILURE;
}
