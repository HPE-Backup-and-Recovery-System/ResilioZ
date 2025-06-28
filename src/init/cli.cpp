#include "init/cli.h"

#include <exception>
#include <iostream>
#include <vector>

#include "systems/all.h"
#include "utils/utils.h"

int RunCLI(int argc, char** argv) {
  // UserIO::ClearTerminal();
  Logger::TerminalLog(
      " ResilioZ (HPE) - Backup and Recovery System in Linux...");
  System* system = nullptr;

  std::vector<std::string> main_menu = {
      "EXIT...", "Backup System", "Restore System", "Others (Services System)"};
  while (true) {
    if (system != nullptr) {
      delete system;
      system = nullptr;
    }

    int choice = UserIO::HandleMenuWithSelect(
        UserIO::DisplayMaxTitle("SYSTEMS", false), main_menu);

    switch (choice) {
      case 0: {
        std::cout << " - Exiting...\n\n";
        delete system;
        system = nullptr;
        return EXIT_SUCCESS;
      }
      case 1: {
        system = new BackupSystem();
        system->Run();
        break;
      }
      case 2: {
        system = new RestoreSystem();
        system->Run();
        break;
      }
      case 3: {
        system = new ServicesSystem();
        system->Run();
        break;
      }
      default: {
        Logger::TerminalLog("Menu Mismatch...", LogLevel::ERROR);
      }
    }
  }
  return EXIT_FAILURE;
}
