#include "utils/prompter.h"

#include <iostream>

#include "utils/user_io.h"
#include "utils/validator.h"

std::string Prompter::PromptUntilValid(
    const std::function<bool(const std::string&)>& validator,
    const std::string& field_name, const std::string& prompt_msg,
    const bool confirm, const bool hide) {
  std::string input;
  do {
    std::cout << prompt_msg;
    if (hide) {
      input = UserIO::GetHiddenInput();
    } else {
      std::getline(std::cin, input);
    }

    if (!validator(input)) {
      std::cout << "Invalid " << field_name << "! Please Try Again...\n";
      continue;
    }

    if (confirm) {
      std::string confirmation;
      std::cout << " -> Confirm " << field_name << ": ";
      if (hide) {
        confirmation = UserIO::GetHiddenInput();
      } else {
        std::getline(std::cin, confirmation);
      }

      if (input != confirmation) {
        std::cout << field_name << "s do not Match! Please Try Again...\n";
        continue;
      }
    }

    break;
  } while (true);

  return input;
}

std::string Prompter::PromptRepoName(const std::string& prompt_msg,
                                     const bool confirm) {
  return Prompter::PromptUntilValid(Validator::IsValidRepoName,
                                    "Repository Name", prompt_msg, confirm);
}

std::string Prompter::PromptPassword(const std::string& prompt_msg,
                                     const bool confirm) {
  return Prompter::PromptUntilValid(Validator::IsValidPassword, "Password",
                                    prompt_msg, confirm, true);
}

std::string Prompter::PromptPath(const std::string& prompt_msg) {
  return Prompter::PromptUntilValid(Validator::IsValidPassword, "Path",
                                    prompt_msg);
}

std::string Prompter::PromptLocalPath(const std::string& prompt_msg) {
  return Prompter::PromptUntilValid(Validator::IsValidLocalPath, "Local Path",
                                    prompt_msg);
}

std::string Prompter::PromptMountPath(const std::string& prompt_msg) {
  return Prompter::PromptUntilValid(Validator::IsValidMountPath, "Mount Path",
                                    prompt_msg);
}

std::string Prompter::PromptSftpPath(const std::string& prompt_msg) {
  return Prompter::PromptUntilValid(Validator::IsValidSftpPath, "SFTP Path",
                                    prompt_msg);
}

std::string Prompter::PromptIpAddress(const std::string& prompt_msg) {
  return Prompter::PromptUntilValid(Validator::IsValidIpAddress, "IP Address",
                                    prompt_msg);
}
