// src/main.cpp
#include <iostream>
#include <vector>
#include <string>
#include <unistd.h>
#include <termios.h>
#include <fcntl.h>
#include <fstream>
#include <sys/stat.h>
#include <pwd.h>
#include "logger.h"
#include "storage_selector.h"
#include "repo_manager.h"

void setNonBlocking(bool enable) {
    struct termios ttystate;
    tcgetattr(STDIN_FILENO, &ttystate);

    if (enable) {
        ttystate.c_lflag &= ~ICANON;
        ttystate.c_lflag &= ~ECHO;
        ttystate.c_cc[VMIN] = 1;
    } else {
        ttystate.c_lflag |= ICANON;
        ttystate.c_lflag |= ECHO;
    }

    tcsetattr(STDIN_FILENO, TCSANOW, &ttystate);
}

int kbhit() {
    struct timeval tv = { 0L, 0L };
    fd_set fds;
    FD_ZERO(&fds);
    FD_SET(STDIN_FILENO, &fds);
    return select(STDIN_FILENO + 1, &fds, NULL, NULL, &tv);
}

char getch() {
    char buf = 0;
    read(STDIN_FILENO, &buf, 1);
    return buf;
}

void displayHPELogo() {
    std::ifstream logoFile("assets/HPE_Black.txt");
    std::string line;
    std::cout << "\033[1;32m";
    if (logoFile.is_open()) {
        while (std::getline(logoFile, line)) {
            std::cout << "    " << line << std::endl;
        }
        logoFile.close();
    } else {
        std::cout << "[HPE LOGO MISSING]" << std::endl;
    }
    std::cout << "\033[0m";
}

void displayTitle() {
    std::ifstream fontFile("assets/font.txt");
    std::string line;
    std::cout << "\033[38;2;230;255;230m";
    if (fontFile.is_open()) {
        while (std::getline(fontFile, line)) {
            std::cout << line << std::endl;
        }
        fontFile.close();
    } else {
        std::cout << "[FONT FILE MISSING]" << std::endl;
    }
    std::cout << "\033[0m";
}

void displayMenu(const std::vector<std::string>& options, int selected, const std::string& currentStorage) {
    system("clear");
    displayHPELogo();
    std::cout << std::endl;
    displayTitle();
    
    // Display current storage location
    if (!currentStorage.empty()) {
        std::cout << "\nCurrent Storage Location: \033[1;32m" << currentStorage << "\033[0m" << std::endl;
    }
    
    std::cout << "\n╭───────────────────────────────╮" << std::endl;
    std::cout << "│   Choose an Option            │" << std::endl;
    std::cout << "╰───────────────────────────────╯" << std::endl;

    for (size_t i = 0; i < options.size(); i++) {
        if (i == selected) {
            std::cout << "  \033[1;32m> " << options[i] << " <\033[0m" << std::endl;
        } else {
            std::cout << "    " << options[i] << std::endl;
        }
    }

    std::cout << "\nUse 'w' to move up, 's' to move down, Enter to select" << std::endl;
}

bool checkAndElevatePremissions() {
    uid_t uid = getuid();
    if (uid != 0) {
        std::cout << "Attempting to run with elevated privileges..." << std::endl;
        std::string cmd = "sudo -E " + std::string(program_invocation_name);
        return system(cmd.c_str()) == 0;
    }
    return true;
}

void displayStorageMenu(const std::vector<std::string>& storages, int selected) {
    system("clear");
    displayHPELogo();
    std::cout << std::endl;
    displayTitle();
    
    std::cout << "\n╭───────────────────────────────╮" << std::endl;
    std::cout << "│   Select Storage Location     │" << std::endl;
    std::cout << "╰───────────────────────────────╯" << std::endl;

    for (size_t i = 0; i < storages.size(); i++) {
        std::string displayPath = storages[i];
        if (displayPath.length() > 50) {
            displayPath = "..." + displayPath.substr(displayPath.length() - 47);
        }
        
        if (i == selected) {
            std::cout << "  \033[1;32m> " << displayPath << " <\033[0m" << std::endl;
        } else {
            std::cout << "    " << displayPath << std::endl;
        }
    }

    std::cout << "\nUse 'w' to move up, 's' to move down, Enter to select, 'q' to quit" << std::endl;
}

std::string selectStorage(StorageSelector& storageSelector) {
    auto storages = storageSelector.getStorages();
    if (storages.empty()) {
        Logger::log("No storage devices found.", LogLevel::WARNING);
        std::cout << "No storage devices found." << std::endl;
        sleep(2);
        return "";
    }

    int selected = 0;
    char key;
    bool selecting = true;
    
    setNonBlocking(true);

    while (selecting) {
        displayStorageMenu(storages, selected);
        
        if (kbhit()) {
            key = getch();
            switch (key) {
                case 'w':
                    if (selected > 0) selected--;
                    break;
                case 's':
                    if (selected < storages.size() - 1) selected++;
                    break;
                case '\n':
                    selecting = false;
                    break;
                case 'q':
                    return "";
            }
        }
        usleep(100000);  // Reduced sleep time for better responsiveness
    }

    setNonBlocking(false);
    return selected >= 0 && selected < storages.size() ? storages[selected] : "";
}

void handleCreateRepo(RepoManager& repoManager) {
    setNonBlocking(false);
    std::cout << "\nEnter repository name: ";
    std::string repoName;
    std::getline(std::cin, repoName);

    if (repoName.empty()) {
        std::cout << "\nRepository name cannot be empty." << std::endl;
        sleep(2);
        return;
    }

    if (!checkAndElevatePremissions()) {
        std::cout << "\nFailed to get necessary permissions." << std::endl;
        sleep(2);
        return;
    }

    if (repoManager.createRepo(repoName)) {
        std::cout << "\nRepository created successfully!" << std::endl;
        sleep(2);
    } else {
        std::cout << "\nPress any key to continue..." << std::endl;
        std::cin.get();
    }
}

std::string handleSelectRepo(RepoManager& repoManager) {
    auto repos = repoManager.listRepos();
    if (repos.empty()) {
        std::cout << "No repositories found." << std::endl;
        sleep(2);
        return "";
    }

    int selected = 0;
    char key;

    while (true) {
        system("clear");
        displayHPELogo();
        std::cout << std::endl;
        displayTitle();
        repoManager.displayRepos();
        std::cout << "\nSelect repository (w/s to move, Enter to select): ";

        if (kbhit()) {
            key = getch();
            if (key == 'w' && selected > 0) {
                selected--;
            } else if (key == 's' && selected < repos.size() - 1) {
                selected++;
            } else if (key == '\n') {
                if (repoManager.selectRepo(repos[selected].name)) {
                    return repos[selected].name;
                }
                return "";
            }
        }
        usleep(100000);
    }
}

int main() {
    Logger::log("Application started", LogLevel::INFO);
    std::string selectedStorage;
    bool running = true;

    while (running) {
        StorageSelector storageSelector;
        storageSelector.scanAndIndexStorages();
        
        selectedStorage = selectStorage(storageSelector);
        if (selectedStorage.empty()) {
            return 1;
        }

        RepoManager repoManager(selectedStorage);
        std::vector<std::string> options = {
            "Create New Repository",
            "Select Existing Repository",
            "Add Files to Repository",
            "Change Storage Location",
            "Exit"
        };

        int selected = 0;
        setNonBlocking(true);
        char key;
        bool submenuRunning = true;

        while (submenuRunning) {
            displayMenu(options, selected, selectedStorage);

            if (kbhit()) {
                key = getch();
                if (key == 'w' && selected > 0) {
                    selected--;
                } else if (key == 's' && selected < options.size() - 1) {
                selected++;
            } else if (key == '\n') {
                switch (selected) {
                        case 0:
                            handleCreateRepo(repoManager);
                            break;
                        case 1: {
                            std::string selectedRepo = handleSelectRepo(repoManager);
                            if (!selectedRepo.empty()) {
                                std::cout << "\nSelected repository: " << selectedRepo << std::endl;
                                sleep(2);
                            }
                        break;
                    }
                        case 2: {
                            if (repoManager.getCurrentRepoPath().empty()) {
                                std::cout << "\nPlease select a repository first." << std::endl;
                                sleep(2);
                            } else {
                                repoManager.handleFileSelection();
                            }
                        break;
                    }
                        case 3:
                            submenuRunning = false;  // Go back to storage selection
                            break;
                        case 4:
                        Logger::log("Exiting system", LogLevel::INFO);
                        setNonBlocking(false);
                        return 0;
                }
            }
        }
            usleep(50000);  // Reduced sleep time for better responsiveness
        }
    }

    setNonBlocking(false);
    return 0;
}
