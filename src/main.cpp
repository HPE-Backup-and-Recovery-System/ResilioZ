#include <exception>
#include <iostream>

#include "services/repository_service.h"
#include "services/scheduler_service.h"

int main(int argc, char** argv) {
  std::cout << "HPE - Backup and Recovery System in Linux..." << std::endl;

  while(true){

    std::cout << "Enter 1 for Repository Management\n";
    std::cout << "Enter 2 for Schedule Management\n";
    int userin;

    std::cin >> userin;
    std::cin.ignore();

    if (userin == 1){
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
    } 
    else if(userin == 2){
      try{
        SchedulerService service;
        service.Run();
      }catch (const std::exception& e) {
        std::cerr << "Error in scheduler service: " << e.what() << std::endl;
        return EXIT_FAILURE;
      } catch (...) {
        std::cerr << "Unknown error Occurred in scheduler service." << std::endl;
        return EXIT_FAILURE;
      }
    }
    else{
      break;
    }
  }



  return EXIT_SUCCESS;
}
