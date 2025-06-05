#include "services/repository_service.h"

#include <exception>
#include <iostream>
#include <limits>

#include "repositories/local_repository.h"
#include "repositories/nfs_repository.h"
#include "repositories/remote_repository.h"
#include "repositories/repository.h"
#include "utils/prompter.h"
#include "utils/repodata_manager.h"
#include "utils/time_util.h"

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

    try {
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
    } catch (std::exception& e) {
      continue;
    }
  }
}

void RepositoryService::CreateNewRepository() {
  std::cout << "\n --- Select Repository Type --- \n\n";

  std::cout << "\t 1. Local Repository\n";
  std::cout << "\t 2. Remote Repository\n";
  std::cout << "\t 3. NFS Repository\n";
  std::cout << "\t 4. Go BACK...\n";

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
      InitNFSRepositoryFromPrompt();
      break;
    case 4:
      return;
    default:
      std::cout << "Invalid Choice!\n";
      break;
  }
}

void RepositoryService::InitLocalRepositoryFromPrompt() {
  std::cout << "\n <<< Local Repository >>> \n\n";
  std::cin.ignore();
  std::string name, path, password;
  name = Prompter::PromptRepoName();
  path = Prompter::PromptLocalPath();
  password = Prompter::PromptPassword(" -> Enter Repository Password: ", true);
  std::cout << std::endl;
  std::string timestamp = TimeUtil::GetCurrentTimestamp();

  LocalRepository* repo = new LocalRepository(path, name, password, timestamp);
  if (repo->Exists()) {
    std::cout << "Repository Already Exists at Given Location!\n";
  } else {
    repo->Initialize();
    std::cout << "Repository Created Successfully!\n";
    std::cout << "\n => NOTE: Please Remember Your Password. "
                 "Losing it means that Your Data is Irrecoverably Lost. \n\n";
  }
  repodata_.AddEntry(
      {name, path, "local", repo->GetHashedPassword(), timestamp});

  delete repository_;
  repository_ = repo;
}

void RepositoryService::InitNFSRepositoryFromPrompt() {
  std::cout << "\n <<< NFS Repository >>> \n\n";
  std::cin.ignore();
  std::string name, path, password;
  name = Prompter::PromptRepoName();
  path = Prompter::PromptMountPath();
  password = Prompter::PromptPassword(" -> Enter Repository Password: ", true);
  std::cout << std::endl;
  std::string timestamp = TimeUtil::GetCurrentTimestamp();

  NFSRepository* repo = new NFSRepository(path, name, password, timestamp);
  if (repo->Exists()) {
    std::cout << "Repository Already Exists at Given Location!\n";
  } else {
    if (repo->Exists()) {
      std::cout << "Repository Already Exists at Given Location!\n";
    } else {
      repo->Initialize();
      std::cout << "Repository Created Successfully!\n";
      std::cout << "\n => NOTE: Please Remember Your Password. "
                   "Losing it means that Your Data is Irrecoverably Lost. \n\n";
    }
  }

  repodata_.AddEntry({name, path, "nfs", repo->GetHashedPassword(), timestamp});

  delete repository_;
  repository_ = repo;
}

void RepositoryService::InitRemoteRepositoryFromPrompt() {
  std::cout << "\n <<< Remote Repository >>> \n\n";
  std::cin.ignore();
  std::string name, path, password;
  name = Prompter::PromptRepoName();
  path = Prompter::PromptSftpPath();
  password = Prompter::PromptPassword(" -> Enter Repository Password: ", true);
  std::cout << std::endl;
  std::string timestamp = TimeUtil::GetCurrentTimestamp();

  RemoteRepository* repo =
      new RemoteRepository(path, name, password, timestamp);
  if (repo->Exists()) {
    std::cout << "Repository Already Exists at Given Location!\n";
  } else {
    repo->Initialize();
    std::cout << "Repository Created Successfully!\n";
    std::cout << "\n => NOTE: Please Remember Your Password. "
                 "Losing it means that Your Data is Irrecoverably Lost. \n\n";
  }
  repodata_.AddEntry(
      {name, path, "remote", repo->GetHashedPassword(), timestamp});

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
  std::string name, path, password;
  name = Prompter::PromptRepoName(" -> Enter Repository Name to DELETE: ");
  path = Prompter::PromptPath();
  password = Prompter::PromptPassword(" -> Enter Repository Password: ", false);

  Repository* repo = nullptr;
  auto entry = repodata_.Find(name, path);
  if (!entry) {
    std::cout << "Repository Not Found in Records!\n";
    return;
  }

  if (entry->password_hash != Repository::GetHashedPassword(password)) {
    std::cout << "Incorrect Password!\n";
    return;
  }

  if (entry->type == "local") {
    repo = new LocalRepository(entry->path, entry->name, password,
                               entry->created_at);
  } else if (entry->type == "nfs") {
    repo = new NFSRepository(entry->path, entry->name, password,
                             entry->created_at);
  } else if (entry->type == "remote") {
    repo = new RemoteRepository(entry->path, entry->name, password,
                                entry->created_at);
  } else {
    std::cout << "Unknown Repository Type!\n";
    return;
  }

  if (!repo->Exists()) {
    std::cout << "Repository Not Found in Path!\n";
    delete repo;
    return;
  }
  repo->Delete();
  std::cout << "Repository Deleted Successfully!\n";

  repodata_.DeleteEntry(name, path);

  delete repository_;
  repository_ = repo;
}
