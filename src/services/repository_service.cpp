#include "services/repository_service.h"

#include <exception>
#include <iostream>
#include <limits>

#include "repositories/local_repository.h"
#include "repositories/nfs_repository.h"
#include "repositories/remote_repository.h"
#include "repositories/repository.h"
#include "utils/error_util.h"
#include "utils/logger.h"
#include "utils/prompter.h"
#include "utils/repodata_manager.h"
#include "utils/time_util.h"
#include "utils/user_io.h"

RepositoryService::RepositoryService() : repository_(nullptr) {
  try {
    // Initialization of RepoData Manager...
  } catch (const std::exception& e) {
    ErrorUtil::LogException(
        e, "Failed to initialize repository data management service");
  }
}

void RepositoryService::Run() { ShowMainMenu(); }

void RepositoryService::Log() {
  Logger::TerminalLog("Repository service is running...");
}

void RepositoryService::ShowMainMenu() {
  std::vector<std::string> main_menu = {
      "Go BACK...", "Create New Repository", "List All Repositories",
      "Use Existing Repository", "Delete a Repository"};

  while (true) {
    int choice = UserIO::HandleMenuWithSelect(
        UserIO::DisplayMaxTitle("Repository Service", false), main_menu);

    try {
      switch (choice) {
        case 0:
          std::cout << "\n - Going Back...\n";
          return;
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
        default:
          Logger::TerminalLog("Menu Mismatch...", LogLevel::ERROR);
      }
    } catch (const std::exception& e) {
      ErrorUtil::LogException(e, "Repository Service");
    }
  }
}

void RepositoryService::CreateNewRepository() {
  std::vector<std::string> menu = {"Go BACK...", "Local Repository",
                                   "NFS Repository", "Remote Repository"};

  while (true) {
    int choice = UserIO::HandleMenuWithSelect(
        UserIO::DisplayMinTitle("Select Repository Type", false), menu);

    try {
      switch (choice) {
        case 0:
          std::cout << "\n - Going Back...\n";
          return;
        case 1:
          InitLocalRepositoryFromPrompt();
          break;
        case 2:
          InitNFSRepositoryFromPrompt();
          break;
        case 3:
          InitRemoteRepositoryFromPrompt();
          break;
        default:
          Logger::TerminalLog("Menu Mismatch...", LogLevel::ERROR);
      }
    } catch (...) {
      ErrorUtil::ThrowNested("Repository creation failure");
    }
  }
}

void RepositoryService::InitLocalRepositoryFromPrompt() {
  UserIO::DisplayTitle("Local Repository");
  std::string name, path, password;
  name = Prompter::PromptRepoName();
  path = Prompter::PromptLocalPath();
  password = Prompter::PromptPassword("Repository Password", true);
  std::cout << std::endl;
  std::string timestamp = TimeUtil::GetCurrentTimestamp();

  LocalRepository* repo = nullptr;
  try {
    repo = new LocalRepository(path, name, password, timestamp);
    if (repo->Exists()) {
      ErrorUtil::ThrowError("Repository already exists at location: " +
                            repo->GetPath());
    } else {
      repo->Initialize();
      Logger::Log("Repository: " + name +
                  " created at location: " + repo->GetPath());
      Logger::TerminalLog(
          "=> Note: Please remember your password. \n"
          "Losing it means that your data is irrecoverably lost.");
    }
    repodata_.AddEntry(
        {name, repo->GetPath(), "local", repo->GetHashedPassword(), timestamp});

    delete repository_;
    repository_ = repo;
  } catch (...) {
    ErrorUtil::ThrowNested("Local repository not initialized");

    if (repo != repository_) {
      delete repo;
    }
  }
}

void RepositoryService::InitNFSRepositoryFromPrompt() {
  UserIO::DisplayTitle("NFS Repository");
  std::string name, path, password;
  name = Prompter::PromptRepoName();
  path = Prompter::PromptMountPath();
  password = Prompter::PromptPassword("Repository Password", true);
  std::cout << std::endl;
  std::string timestamp = TimeUtil::GetCurrentTimestamp();

  NFSRepository* repo = nullptr;
  try {
    repo = new NFSRepository(path, name, password, timestamp);
    if (repo->Exists()) {
      ErrorUtil::ThrowError("Repository already exists at location: " +
                            repo->GetPath());
    } else {
      repo->Initialize();
      Logger::Log("Repository: " + name +
                  " created at location: " + repo->GetPath());
      Logger::TerminalLog(
          "=> Note: Please remember your password. \n"
          "Losing it means that your data is irrecoverably lost.");
    }
    repodata_.AddEntry(
        {name, repo->GetPath(), "nfs", repo->GetHashedPassword(), timestamp});

    SetRepository(repo);
  } catch (const std::exception& e) {
    ErrorUtil::ThrowNested("NFS repository not initialized");

    if (repo != repository_) {
      delete repo;
    }
  }
}

void RepositoryService::InitRemoteRepositoryFromPrompt() {
  UserIO::DisplayTitle("Remote Repository");
  std::string name, path, password;
  name = Prompter::PromptRepoName();
  path = Prompter::PromptSftpPath();
  password = Prompter::PromptPassword("Repository Password", true);
  std::cout << std::endl;
  std::string timestamp = TimeUtil::GetCurrentTimestamp();

  RemoteRepository* repo = nullptr;
  try {
    repo = new RemoteRepository(path, name, password, timestamp);
    if (repo->Exists()) {
      ErrorUtil::ThrowError("Repository already exists at location: " +
                            repo->GetPath());
    } else {
      repo->Initialize();
      Logger::Log("Repository: " + name +
                  " created at location: " + repo->GetPath());
      Logger::TerminalLog(
          "=> Note: Please remember your password. \n"
          "Losing it means that your data is irrecoverably lost.");
    }
    repodata_.AddEntry({name, repo->GetPath(), "remote",
                        repo->GetHashedPassword(), timestamp});

    SetRepository(repo);
  } catch (const std::exception& e) {
    ErrorUtil::ThrowNested("Remote repository not initialized");

    if (repo != repository_) {
      delete repo;
    }
  }
}

void RepositoryService::ListRepositories() {
  try {
    const auto repos = repodata_.GetAll();
    if (repos.empty()) {
      UserIO::DisplayMinTitle("No repositories found");
      return;
    }

    UserIO::DisplayMinTitle("Repository List");
    for (const auto& repo : repos) {
      std::cout << " - Name: " << repo.name << " ["
                << RepodataManager::GetFormattedTypeString(repo.type) << "]"
                << "\n\t Path: " << repo.path
                << "\n\t Created at: " << repo.created_at << "\n\n";
    }
  } catch (const std::exception& e) {
    ErrorUtil::ThrowNested("Unable to fetch available repositories");
  }
}

Repository* RepositoryService::UseExistingRepository() {
  std::string name, path, password;
  name = Prompter::PromptRepoName("Existing Repository Name");
  path = Prompter::PromptPath();
  password = Prompter::PromptPassword("Password", false);

  Repository* repo = nullptr;
  auto entry = repodata_.GetEntry(name, path);
  try {
    if (!entry) {
      Logger::TerminalLog("Please verify repository name and path...",
                          LogLevel::WARNING);
      ErrorUtil::ThrowError("Repository not found in data file");
    }

    if (entry->password_hash != Repository::GetHashedPassword(password)) {
      ErrorUtil::ThrowError("Incorrect password entered");
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
      ErrorUtil::ThrowError("Encountered unknown repository type: " +
                            entry->type);
    }

    if (!repo->Exists()) {
      delete repo;
      ErrorUtil::ThrowError("Repository not found in path: " + repo->GetPath());
    }

    Logger::Log("Repository: " + name + " [" +
                RepodataManager::GetFormattedTypeString(entry->type) +
                "] loaded from: " + repo->GetPath());
    SetRepository(repo);
    return repo;

  } catch (const std::exception& e) {
    ErrorUtil::ThrowNested("Repository usage failure");
    delete repo;
    return nullptr;
  }
}

void RepositoryService::DeleteRepository() {
  std::string name, path, password;
  name = Prompter::PromptRepoName("Repository Name to DELETE");
  path = Prompter::PromptPath();
  password = Prompter::PromptPassword("Password", false);

  Repository* repo = nullptr;
  auto entry = repodata_.GetEntry(name, path);
  try {
    if (!entry) {
      Logger::TerminalLog("Please verify repository name and path...",
                          LogLevel::WARNING);
      ErrorUtil::ThrowError("Repository not found in data file");
    }

    if (entry->password_hash != Repository::GetHashedPassword(password)) {
      ErrorUtil::ThrowError("Incorrect password entered");
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
      ErrorUtil::ThrowError("Encountered unknown repository type: " +
                            entry->type);
    }

    if (!repo->Exists()) {
      delete repo;
      ErrorUtil::ThrowError("Repository not found in path: " + repo->GetPath());
    }
    repo->Delete();

    repodata_.DeleteEntry(name, path);
    Logger::Log("Repository: " + name + " [" +
                RepodataManager::GetFormattedTypeString(entry->type) +
                "] deleted from location: " + repo->GetPath());

    SetRepository(repo);
  } catch (const std::exception& e) {
    ErrorUtil::ThrowNested("Repository deletion failure");
  }
}

void RepositoryService::SetRepository(Repository* new_repo) {
  delete repository_;
  repository_ = new_repo;
}
