#include "utils/user_io.h"

#include <termios.h>
#include <unistd.h>

#include <algorithm>
#include <iostream>
#include <limits>

#include "utils/logger.h"

void UserIO::ClearTerminal() {
  if (system("clear") != 0) {
    Logger::TerminalLog("Failed to clear terminal...", LogLevel::WARNING);
  }
}

char UserIO::ReadRawKey() {
  termios oldt, newt;
  char ch;
  tcgetattr(STDIN_FILENO, &oldt);
  newt = oldt;
  newt.c_lflag &= ~(ICANON | ECHO);
  tcsetattr(STDIN_FILENO, TCSANOW, &newt);

  if (read(STDIN_FILENO, &ch, 1) != 1) {
    Logger::TerminalLog("Failed to read character from stdin...",
                        LogLevel::WARNING);
  }

  tcsetattr(STDIN_FILENO, TCSANOW, &oldt);
  return ch;
}

std::string UserIO::ReadHiddenInput() {
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

void UserIO::DisplayMenu(const std::string& header,
                         const std::vector<std::string>& options,
                         const std::string& footer, bool index_mode,
                         int active_option) {
  const std::string indent(4, ' ');
  int menu_size = static_cast<int>(options.size());
  std::cout << header;

  if (index_mode) {
    for (int i = 0; i < menu_size; i++) {
      auto index = (i + 1) % menu_size;
      std::cout << indent << " " << index << ". " << options[index]
                << std::endl;
    }
    if (!footer.empty()) {
      std::cout << indent << footer;
    }
  } else {
    for (int i = 0; i < menu_size; i++) {
      auto index = (i + 1) % menu_size;
      if (index == active_option) {
        std::cout << "   \033[1;32m> " << options[index] << " <\033[0m"
                  << std::endl;
      } else {
        std::cout << indent << " " << options[index] << std::endl;
      }
    }
    if (!footer.empty()) {
      std::cout << indent << footer << std::ends;
    }
  }
}

int UserIO::HandleMenuWithInput(const std::string& header,
                                const std::vector<std::string>& options,
                                const std::string& footer) {
  int menu_size = static_cast<int>(options.size());
  if (!menu_size) {
    Logger::TerminalLog("Menu options are empty!", LogLevel::ERROR);
    return -1;
  }

  int choice = -1;
  while (true) {
    try {
      const auto footer_ = "=> Enter " + footer + ": ";
      DisplayMenu(header, options, footer_);

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

int UserIO::HandleMenuWithSelect(const std::string& header,
                                 const std::vector<std::string>& options,
                                 const std::string& footer) {
  int menu_size = static_cast<int>(options.size());
  if (!menu_size) {
    Logger::TerminalLog("Menu options are empty!", LogLevel::ERROR);
    return -1;
  }

  int choice = 1;
  bool clear = false;

  while (true) {
    try {
      const auto footer_ = footer +
                           "\n [ Use Arrow Keys to Navigate | ENTER to Select |"
                           " q/ESC to Return ] \n\n";

      if (clear) {
        ClearPreviousMenuLines(CountMenuLines(header, options, footer_));
      } else {
        clear = true;
      }
      DisplayMenu(header, options, footer_, false, choice);

      char first_char = UserIO::ReadRawKey();
      if (first_char == '\033') {
        char second_char = UserIO::ReadRawKey();
        if (second_char == '[') {
          char third_char = UserIO::ReadRawKey();
          if (third_char == 'A' || third_char == 'D') {
            choice = (choice - 1 + menu_size) % menu_size;
          } else if (third_char == 'B' || third_char == 'C') {
            choice = (choice + 1) % menu_size;
          }
        } else {
          return 0;
        }
      } else if (first_char == '\n') {
        return choice;
      } else if (first_char == 'q' || first_char == 'Q') {
        return 0;
      }
    } catch (...) {
      Logger::TerminalLog("Menu selection failure...", LogLevel::ERROR);
    }
  }
  return choice;
}

void UserIO::ClearPreviousMenuLines(int lines) {
  for (int i = 0; i < lines; i++) {
    std::cout << "\033[A\033[2K";
  }
}

int UserIO::CountMenuLines(const std::string& header,
                           const std::vector<std::string>& options,
                           const std::string& footer) {
  int header_lines = std::count(header.begin(), header.end(), '\n');
  int footer_lines = std::count(footer.begin(), footer.end(), '\n');
  int option_lines = static_cast<int>(options.size());
  int extras = 0;

  return header_lines + option_lines + footer_lines + extras;
}
