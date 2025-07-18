#include "utils/prompter.h"

#include <iostream>

#include "utils/logger.h"
#include "utils/user_io.h"
#include "utils/validator.h"

std::string Prompter::PromptUntilValid(
    const std::function<bool(const std::string&)>& validator,
    const std::string& field_name, const std::string& field_prompt,
    const bool confirm, const bool hide) {
  std::string input;
  do {
    std::cout << " -> Enter " << field_prompt << ": ";
    if (hide) {
      input = UserIO::ReadHiddenInput();
    } else {
      std::getline(std::cin, input);
    }

    if (!validator(input)) {
      if (field_name == "Password") {
        Logger::TerminalLog("Password requirements not met. Password must contain:\n"
                          "- At least 8 characters and no more than 64 characters\n"
                          "- At least one uppercase letter (A-Z)\n"
                          "- At least one lowercase letter (a-z)\n"
                          "- At least one number (0-9)\n"
                          "- At least one special character (!@#$%^&* etc.)\n",
                          LogLevel::ERROR);
      } else {
        Logger::TerminalLog("Invalid " + field_name + "! Please try again...\n",
                          LogLevel::ERROR);
      }
      continue;
    }

    if (confirm) {
      std::string confirmation;
      std::cout << " -> Confirm " << field_name << ": ";
      if (hide) {
        confirmation = UserIO::ReadHiddenInput();
      } else {
        std::getline(std::cin, confirmation);
      }

      if (input != confirmation) {
        Logger::TerminalLog(
            field_name + "s do not match! Please try again...\n",
            LogLevel::ERROR);
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
  return Prompter::PromptUntilValid(Validator::IsValidPath, "Path",
                                    prompt_msg);
}

std::string Prompter::PromptLocalPath(const std::string& prompt_msg) {
  return Prompter::PromptUntilValid(Validator::IsValidLocalPath, "Local Path",
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

std::string Prompter::PromptCronString(const std::string& prompt_msg){
  return Prompter::PromptUntilValid(Validator::IsValidCronString, "Cron String",
                                    prompt_msg);
}

std::string Prompter::PromptScheduleId(const std::string& prompt_msg){
  return Prompter::PromptUntilValid(Validator::IsValidScheduleId, "Schedule ID",
                                    prompt_msg);
}

std::string Prompter::PromptInput(const std::string& prompt_msg) {
  return Prompter::PromptUntilValid(Validator::Any, "Input", prompt_msg);
}

std::string Prompter::PromptNfsPath(const std::string& prompt_msg) {
  return Prompter::PromptUntilValid(Validator::IsValidNfsPath, "NFS Path", prompt_msg);
}