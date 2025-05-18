#include "services/repository_service.h"

#include <iostream>
#include <limits>

#include "repositories/local_repository.h"
#include "repositories/remote_repository.h"
#include "utils/repodata_manager.h"
#include "utils/time_util.h"

// TODO: User Prompt Helpers...

RepositoryService::RepositoryService() : repository_(nullptr) {}

void RepositoryService::Run() { ShowMainMenu(); }

void RepositoryService::Log() {
  std::cout << " *** Logging NOT Implemented Yet! *** " << std::endl;
}

void RepositoryService::ShowMainMenu() {
  while (true) {
    std::cout << "\n ==== Repository Service ==== \n\n";
    std::cout << "\t 1. Create NEW Repository\n";
    std::cout << "\t 2. List ALL Repositories\n";
    std::cout << "\t 3. Use EXISTING Repository\n";
    std::cout << "\t 4. DELETE a Repository\n";
    std::cout << "\t 5. EXIT... \n";
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
        CreateNewRepository();
        break;
      case 2:
        ListRepositories();
        break;
      case 3:
        UseExistingRepository();
        break;
      case 4:
        DeleteRepository();
        break;
      case 5:
        std::cout << "Exiting...\n";
        return;
      default:
        std::cout << "Invalid Option! Retry...\n";
        break;
    }
  }
}

void RepositoryService::CreateNewRepository() {
  std::cout << "\n --- Select Repository Type --- \n\n";
  std::cout << "\t 1. Local Repository\n";
  std::cout << "\t 2. Remote Repository\n";
  std::cout << "\t 3. Go BACK...\n";
  std::cout << "\n\t => Enter Choice: ";

  int choice;
  std::cin >> choice;

  switch (choice) {
    case 1:
      InitLocalRepositoryFromPrompt();
      break;
    case 2:
      InitRemoteRepositoryFromPrompt();
      break;
    case 3:
      return;
    default:
      std::cout << "Invalid Choice!\n";
      break;
  }
}

void RepositoryService::InitLocalRepositoryFromPrompt() {
  std::cout << "\n <<< Local Repository >>> \n\n";
  std::cin.ignore();
  std::string path, name;
  std::cout << " -> Enter Repository Name: ";
  std::getline(std::cin, name);
  std::cout << " -> Enter Base Path: ";
  std::getline(std::cin, path);
  std::cout << std::endl;
  std::string timestamp = time_util::GetCurrentTimestamp();

  LocalRepository* repo = new LocalRepository(path, name, timestamp);
  if (repo->Exists()) {
    std::cout << "Repository Already Exists at Given Location!\n";
  } else {
    repo->Initialize();
    std::cout << "Repository Created Successfully!\n";
  }

  repodata_.AddEntry({name, path, "local", timestamp});

  delete repository_;
  repository_ = repo;
}

void RepositoryService::InitRemoteRepositoryFromPrompt() {
  std::cout << "\n <<< Remote Repository >>> \n\n";
  std::cin.ignore();
  std::string ip, username, password, name, path;
  std::cout << " -> Enter IP Address: ";
  std::getline(std::cin, ip);
  std::cout << " -> Enter Username: ";
  std::getline(std::cin, username);
  std::cout << " -> Enter Password: ";
  std::getline(std::cin, password);
  std::cout << " -> Enter Repository Name: ";
  std::getline(std::cin, name);
  std::cout << " -> Enter Base Path: ";
  std::getline(std::cin, path);
  std::cout << std::endl;
  std::string timestamp = time_util::GetCurrentTimestamp();

  RemoteRepository* repo = new RemoteRepository(ip, username, password, name, path, timestamp);
  repo->Initialize();

  repodata_.AddEntry({name, ip, "remote", timestamp});

  delete repository_;
  repository_ = repo;
}

void RepositoryService::ListRepositories() {
  const auto repos = repodata_.GetAll();
  if (repos.empty()) {
    std::cout << "No Repositories Recorded.\n";
    return;
  }
  std::cout << "\n --- List of Repositories --- \n\n";
  for (const auto& repo : repos) {
    std::cout << " - Name: " << repo.name << " [ " << repo.type << " ]"
              << "\n\t Path: " << repo.path
              << "\n\t Created: " << repo.created_at << "\n\n";
  }
  std::cout << std::endl;
}

void RepositoryService::UseExistingRepository() {
  std::cout << " *** Usage of Existing Repositories NOT Implemented Yet! *** "
            << std::endl;
}

void RepositoryService::DeleteRepository() {
  std::cin.ignore();
  std::string name;
  std::cout << "\n -> Enter Repository Name to DELETE: ";
  std::getline(std::cin, name);
  std::cout << " -> Confirm Repository Name to DELETE: ";
  std::string confirm;
  std::getline(std::cin, confirm);

  if (confirm != name) {
    std::cout << "Confirmation FAILED! Deletion Aborted.\n";
    return;
  }

  // TODO: Actual Deletion...

  if (repodata_.DeleteEntry(name)) {
    std::cout << "Repository '" << name << "' Deleted.\n";
  } else {
    std::cout << "Repository Not Found! Aborting...\n";
  }
}

std::shared_ptr<Repository> RepositoryService::LoadFromConfig(
    const std::string& config_path) {
  std::cout << " *** LoadFromConfig() NOT Implemented Yet! *** " << std::endl;
  return nullptr;
}

