#include "utils/user_io.h"

#include <termios.h>
#include <unistd.h>

#include <iostream>
#include <limits>

#include "utils/logger.h"

std::string UserIO::GetHiddenInput() {
  std::string input;
  termios oldt, newt;

  tcgetattr(STDIN_FILENO, &oldt);
  newt = oldt;

  newt.c_lflag &= ~ECHO;
  tcsetattr(STDIN_FILENO, TCSANOW, &newt);

  char ch;
  while ((ch = getchar()) != '\n') {
    if (ch == 127) {
      if (!input.empty()) {
        std::cout << "\b \b";
        input.pop_back();
      }
    } else {
      input += ch;
      std::cout << '*';
    }
  }
  std::cout << '\n';

  tcsetattr(STDIN_FILENO, TCSANOW, &oldt);

  return input;
}

std::string UserIO::DisplayTitle(const std::string& message, bool print) {
  std::string formatted_message = "\n <<< " + message + " >>> \n\n";
  if (print) {
    std::cout << formatted_message;
  }
  return formatted_message;
}

std::string UserIO::DisplayMinTitle(const std::string& message, bool print) {
  std::string formatted_message = "\n --- " + message + " --- \n\n";
  if (print) {
    std::cout << formatted_message;
  }
  return formatted_message;
}

std::string UserIO::DisplayMaxTitle(const std::string& message, bool print) {
  std::string formatted_message = "\n === " + message + " === \n\n";
  if (print) {
    std::cout << formatted_message;
  }
  return formatted_message;
}

void UserIO::DisplayMenu(const std::string& menu_header,
                         const std::vector<std::string>& menu_options,
                         const std::string& menu_footer) {}

int UserIO::HandleMenuWithInput(const std::string& menu_header,
                                const std::vector<std::string>& menu_options,
                                const std::string& menu_footer) {
  int menu_size = (int)menu_options.size();
  if (!menu_size) {
    Logger::TerminalLog("Menu options are empty!", LogLevel::ERROR);
    return -1;
  }

  int choice = -1;
  std::cout << menu_header;
  while (true) {
    try {
      for (int i = 1; i < menu_size; i++) {
        std::cout << "     " << i << ". " << menu_options[i] << std::endl;
      }
      std::cout << "     0. " << menu_options[0] << std::endl;
      std::cout << "    => Enter " + menu_footer + ": ";

      std::cin >> choice;
      if (std::cin.fail()) {
        std::cin.clear();
        Logger::TerminalLog("Invalid input! Please enter a number...",
                            LogLevel::ERROR);
        continue;
      }
      std::cin.ignore(std::numeric_limits<std::streamsize>::max(), '\n');

      if (-1 < choice && choice < menu_size) {
        break;
      }

      Logger::TerminalLog("Choice is out of range...");
    } catch (...) {
      Logger::TerminalLog("Invalid choice! Please try again...",
                          LogLevel::ERROR);
    }
  }
  return choice;
}

int UserIO::HandleMenuWithSelect(const std::string& menu_header,
                                 const std::vector<std::string>& menu_options,
                                 const std::string& menu_footer) {
  return 0;
}
