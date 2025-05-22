#include <exception>
#include <iostream>

#include "services/repository_service.h"

int main(int argc, char** argv) {
  std::cout << "HPE - Backup and Recovery System in Linux..." << std::endl;

  try {
    RepositoryService service;
    service.Run();
  } catch (const std::exception& e) {
    std::cerr << "ERROR in a Service: " << e.what() << std::endl;
    return EXIT_FAILURE;
  } catch (...) {
    std::cerr << "Unknown ERROR Occurred in a Service." << std::endl;
    return EXIT_FAILURE;
  }

  return EXIT_SUCCESS;
}
