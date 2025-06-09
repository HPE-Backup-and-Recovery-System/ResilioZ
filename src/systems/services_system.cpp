#include "systems/services_system.h"

#include <iostream>
#include <vector>

#include "services/all.h"
#include "utils/utils.h"

ServicesSystem::ServicesSystem() {
  services_.push_back(new RepositoryService());
  services_.push_back(new SchedulerService());
}

ServicesSystem::~ServicesSystem() {
  for (auto* service : services_) {
    if (service) {
      delete service;
    }
  }
  services_.clear();
}

void ServicesSystem::Run() {
  std::vector<std::string> system_menu = {"Go BACK... ", "Repository Service",
                                          "Schedule Service"};
  while (true) {
    int choice = UserIO::HandleMenuWithSelect(
        UserIO::DisplayMaxTitle("SYSTEM - SERVICES", false), system_menu);

    switch (choice) {
      case 0: {
        std::cout << " - Going Back...\n";
        return;
      }
      case 1: {
        services_[0]->Run();
        break;
      }
      case 2: {
        services_[1]->Run();
        break;
      }
      default: {
        Logger::TerminalLog("Menu Mismatch...", LogLevel::ERROR);
      }
    }
  }
}

void ServicesSystem::Log() {
  Logger::TerminalLog("Services system is up and running... \n");

  std::ostringstream services;
  services << "Available System Services: "
           << "\n - Repository Service"
           << "\n - Scheduler Service" << std::endl;
  Logger::TerminalLog(services.str());
}
