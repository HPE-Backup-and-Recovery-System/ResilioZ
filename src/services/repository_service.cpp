#include "services/repository_service.h"

#include <iostream>
#include <limits>

#include "repositories/local_repository.h"
#include "repositories/remote_repository.h"

RepositoryService::RepositoryService() : repository_(nullptr) {}

void RepositoryService::Run() { ShowMainMenu(); }

void RepositoryService::Log() {
  std::cout << " *** Logging NOT Implemented Yet! *** " << std::endl;
}

void RepositoryService::ShowMainMenu() {
  while (true) {
    std::cout << "\n ==== Repository Service ==== \n\n";
    std::cout << "\t 1. Create NEW Repository\n";
    std::cout << "\t 2. Use Existing Repository\n";
    std::cout << "\t 3. EXIT... \n";
    std::cout << "\n\t => Enter Choice: ";

    int choice;
    std::cin >> choice;

    if (std::cin.fail()) {
      std::cin.clear();
      std::cin.ignore(std::numeric_limits<std::streamsize>::max(), '\n');
      std::cout << "Invalid Input! Retry...\n";
      continue;
    }

    switch (choice) {
      case 1:
        HandleNewRepository();
        break;
      case 2:
        HandleExistingRepository();
        break;
      case 3:
        std::cout << "Exiting...\n";
        return;
      default:
        std::cout << "Invalid Option! Retry...\n";
        break;
    }
  }
}

void RepositoryService::HandleNewRepository() {
  std::cout << "\n --- Select Repository Type --- \n\n";
  std::cout << "\t 1. Local Repository\n";
  std::cout << "\t 2. Remote Repository (via SSH)\n";
  std::cout << "\t 3. Go BACK...\n";
  std::cout << "\n\t => Enter Choice: ";

  int choice;
  std::cin >> choice;

  switch (choice) {
    case 1:
      HandleLocalRepository();
      break;
    case 2:
      HandleRemoteRepository();
      break;
    case 3:
      return;
    default:
      std::cout << "Invalid Choice!\n";
      break;
  }
}

void RepositoryService::HandleExistingRepository() {
  std::cout << " *** Usage of Existing Repositories NOT Implemented Yet! *** "
            << std::endl;
}

void RepositoryService::HandleLocalRepository() {
  std::cout << "\n <<< Local Repository >>> \n\n";
  std::cin.ignore();
  std::string path, name;
  std::cout << " -> Enter Base Path: ";
  std::getline(std::cin, path);
  std::cout << " -> Enter Repository Name: ";
  std::getline(std::cin, name);
  std::cout << std::endl;

  LocalRepository* repo = new LocalRepository(path, name);
  if (repo->Exists()) {
    std::cout << "Repository Already Exists at Given Location!\n";
  } else {
    repo->Initialize();
    std::cout << "Repository Created Successfully!\n";
  }

  delete repository_;
  repository_ = repo;
}

void RepositoryService::HandleRemoteRepository() {
  std::cout << "\n <<< Remote Repository >>> \n\n";
  std::cin.ignore();
  std::string ip, username, password, name;
  std::cout << " -> Enter IP Address: ";
  std::getline(std::cin, ip);
  std::cout << " -> Enter Username: ";
  std::getline(std::cin, username);
  std::cout << " -> Enter Password: ";
  std::getline(std::cin, password);
  std::cout << " -> Enter Repository Name: ";
  std::getline(std::cin, name);
  std::cout << std::endl;

  RemoteRepository* repo = new RemoteRepository(ip, username, password, name);
  repo->Initialize();

  delete repository_;
  repository_ = repo;
}
