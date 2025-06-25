#include "services/repository_service.h"

#include <exception>
#include <fstream>
#include <iostream>
#include <limits>

#include "nlohmann/json.hpp"
#include "repositories/all.h"
#include "utils/utils.h"

RepositoryService::RepositoryService() : repository_(nullptr) {
  try {
    repodata_mgr = new RepodataManager();
  } catch (const std::exception& e) {
    ErrorUtil::LogException(
        e, "Failed to initialize repository data management service");
  }
}

RepositoryService::~RepositoryService() {
  if (repository_) delete repository_;
  delete repodata_mgr;
}

void RepositoryService::Run() { ShowMainMenu(); }

void RepositoryService::Log() {
  Logger::TerminalLog("Repository service is running...");
}

void RepositoryService::ShowMainMenu() {
  std::vector<std::string> main_menu = {
      "Go BACK...", "Create New Repository", "List All Repositories",
      "Fetch Existing Repository", "Delete a Repository"};

  while (true) {
    int choice = UserIO::HandleMenuWithSelect(
        UserIO::DisplayMaxTitle("Repository Service", false), main_menu);

    try {
      switch (choice) {
        case 0:
          std::cout << " - Going Back...\n";
          return;
        case 1:
          CreateNewRepository();
          break;
        case 2:
          ListRepositories();
          break;
        case 3:
          SelectExistingRepository();
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

bool RepositoryService::CreateNewRepository(bool loop) {
  std::vector<std::string> menu = {"Go BACK...", "Local Repository",
                                   "NFS Repository", "Remote Repository"};

  do {
    int choice = UserIO::HandleMenuWithSelect(
        UserIO::DisplayMinTitle("Select Repository Type", false), menu);

    try {
      switch (choice) {
        case 0:
          std::cout << " - Going Back...\n";
          return false;
        case 1:
          InitLocalRepositoryFromPrompt();
          return true;
        case 2:
          InitNFSRepositoryFromPrompt();
          return true;
        case 3:
          InitRemoteRepositoryFromPrompt();
          return true;
        default:
          Logger::TerminalLog("Menu Mismatch...", LogLevel::ERROR);
          return false;
      }
    } catch (...) {
      ErrorUtil::ThrowNested("Repository creation failure");
      return false;
    }
  } while (loop);

  return false;
}

void RepositoryService::InitLocalRepositoryFromPrompt() {
  UserIO::DisplayTitle("Local Repository");
  std::string name, path, password;
  name = Prompter::PromptRepoName();
  path = Prompter::PromptLocalPath();
  if (path.empty()) {
    path = ".";
  }
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
    repodata_mgr->AddEntry(
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
  std::string name, server_ip, server_backup_path, client_mount_path, password;
  name = Prompter::PromptRepoName();
  server_ip = Prompter::PromptIpAddress("NFS Server IP Address");
  server_backup_path =
      Prompter::PromptPath("Server Backup Path (e.g., /backup_pool)");
  password = Prompter::PromptPassword("Repository Password", true);
  std::cout << std::endl;
  std::string timestamp = TimeUtil::GetCurrentTimestamp();

  Logger::TerminalLog("Repository will be mounted at: " + client_mount_path +
                      "/" + name);

  NFSRepository* repo = nullptr;
  try {
    repo = new NFSRepository(server_ip, server_backup_path, name, password,
                             timestamp);
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
    repodata_mgr->AddEntry(
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
    repodata_mgr->AddEntry({name, repo->GetPath(), "remote",
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
    const auto repos = repodata_mgr->GetAll();
    if (repos.empty()) {
      UserIO::DisplayMinTitle("No repositories found");
      return;
    }

    UserIO::DisplayMinTitle("Repository List");
    for (const auto& repo : repos) {
      std::cout << " - Name: " << repo.name << " ["
                << Repository::GetFormattedTypeString(repo.type) << "]"
                << "\n\t Path: " << repo.path
                << "\n\t Created at: " << repo.created_at << "\n\n";
    }
  } catch (const std::exception& e) {
    ErrorUtil::ThrowNested("Unable to fetch available repositories");
  }
}

Repository* RepositoryService::SelectExistingRepository() {
  Repository* repo = nullptr;
  const auto repodata_entries = repodata_mgr->GetAll();

  if (repodata_entries.empty()) {
    UserIO::DisplayMinTitle("No repositories found");
    Logger::TerminalLog("Please create a repository...", LogLevel::WARNING);
    return nullptr;
  }

  std::vector<std::string> repo_menu = {"Go BACK..."};
  for (const auto& repo : repodata_entries) {
    repo_menu.push_back(
        Repository::GetRepositoryInfoString(repo.name, repo.type, repo.path));
  }

  try {
    int repo_choice = UserIO::HandleMenuWithSelect(
        UserIO::DisplayMinTitle("Select Repository", false), repo_menu);

    if (repo_choice == 0) {
      std::cout << " - Going Back...\n";
      return nullptr;
    }

    const auto selected_repo = repodata_entries[repo_choice - 1];
    std::string password =
        Prompter::PromptPassword("Repository Password", false);

    if (selected_repo.password_hash !=
        Repository::GetHashedPassword(password)) {
      ErrorUtil::ThrowError("Incorrect password entered");
    }

    if (selected_repo.type == "local") {
      repo = new LocalRepository(selected_repo.path, selected_repo.name,
                                 password, selected_repo.created_at);
    } else if (selected_repo.type == "nfs") {
      // Read server IP and backup path from config file
      std::string config_path = selected_repo.path + "/config.json";
      std::ifstream config_file(config_path);
      if (!config_file.is_open()) {
        ErrorUtil::ThrowError("Failed to read NFS config file: " + config_path);
      }
      nlohmann::json config;
      config_file >> config;
      config_file.close();

      repo = new NFSRepository(
          config.at("server_ip"), config.at("server_backup_path"),
          selected_repo.name, password, selected_repo.created_at);
    } else if (selected_repo.type == "remote") {
      repo = new RemoteRepository(selected_repo.path, selected_repo.name,
                                  password, selected_repo.created_at);
    } else {
      ErrorUtil::ThrowError("Encountered unknown repository type: " +
                            selected_repo.type);
    }

    if (!repo->Exists()) {
      delete repo;
      ErrorUtil::ThrowError("Repository not found in path: " + repo->GetPath());
    }

    Logger::Log("Repository: " + selected_repo.name + " [" +
                Repository::GetFormattedTypeString(selected_repo.type) +
                "] loaded from: " + repo->GetPath());
    SetRepository(repo);
    return repo;

  } catch (const std::exception& e) {
    ErrorUtil::ThrowNested("Unable to fetch repository");
    delete repo;
    return nullptr;
  }
}

void RepositoryService::DeleteRepository() {
  try {
    const auto repo = SelectExistingRepository();
    if (repo == nullptr) {
      return;
    }

    if (!repo->Exists()) {
      repodata_mgr->DeleteEntry(repo->GetName(), repo->GetPath());
      Logger::Log("Deleted entry for repository: " +
                  repo->GetRepositoryInfoString() + " as it does not exist");
      delete repo;
      ErrorUtil::ThrowError("Repository not found in path: " + repo->GetPath());
    }
    repo->Delete();

    repodata_mgr->DeleteEntry(repo->GetName(), repo->GetPath());
    Logger::Log("Repository: " + repo->GetName() + " [" +
                Repository::GetFormattedTypeString(repo->GetType()) +
                "] deleted from location: " + repo->GetPath());

    SetRepository(repo);
  } catch (const std::exception& e) {
    ErrorUtil::ThrowNested("Repository deletion failure");
  }
}

Repository* RepositoryService::GetRepository() { return repository_; }

void RepositoryService::SetRepository(Repository* new_repo) {
  delete repository_;
  repository_ = new_repo;
}

std::vector<RepoEntry> RepositoryService::GetAllRepositories() const {
  return repodata_mgr->GetAll();
}
