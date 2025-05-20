#include "utils/prompter.h"

#include <iostream>

#include "utils/validator.h"

namespace Prompter {

std::string PromptUntilValid(
    const std::function<bool(const std::string&)>& validator,
    const std::string& field_name, const std::string& prompt_msg,
    const bool confirm) {
  std::string input;
  do {
    std::cout << prompt_msg;
    std::getline(std::cin, input);
    if (!validator(input)) {
      std::cout << "Invalid " << field_name << "! Please Try Again...\n";
      continue;
    }
    if (confirm) {
      std::string confirmation;
      std::cout << " -> Confirm " << field_name << ": ";
      std::getline(std::cin, confirmation);
      if (input != confirmation) {
        std::cout << field_name << "s do not Match! Please Try Again...\n";
        continue;
      }
    }
    break;
  } while (true);
  return input;
}

std::string PromptRepoName(const std::string& prompt_msg, const bool confirm) {
  return PromptUntilValid(Validator::IsValidRepoName, "Repository Name",
                          prompt_msg, confirm);
}

std::string PromptPassword(const std::string& prompt_msg, const bool confirm) {
  return PromptUntilValid(Validator::IsValidPassword, "Password", prompt_msg,
                          confirm);
}

std::string PromptPath(const std::string& prompt_msg) {
  return PromptUntilValid(Validator::IsValidPassword, "Path", prompt_msg);
}

std::string PromptLocalPath(const std::string& prompt_msg) {
  return PromptUntilValid(Validator::IsValidLocalPath, "Local Path",
                          prompt_msg);
}

std::string PromptSftpPath(const std::string& prompt_msg) {
  return PromptUntilValid(Validator::IsValidSftpPath, "SFTP Path", prompt_msg);
}

std::string PromptIpAddress(const std::string& prompt_msg) {
  return PromptUntilValid(Validator::IsValidIpAddress, "IP Address",
                          prompt_msg);
}

}  // namespace Prompter
