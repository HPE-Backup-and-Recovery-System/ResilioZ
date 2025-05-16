#include <iostream>

#include "services/repository_service.h"

int main(int argc, char **argv) {
  std::cout << "HPE - Backup and Recovery System in Linux..." << std::endl;

  RepositoryService service;
  service.Run();

  return 0;
}