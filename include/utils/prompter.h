#ifndef PROMPTER_H_
#define PROMPTER_H_

#include <functional>
#include <string>

namespace Prompter {

std::string PromptUntilValid(
    const std::function<bool(const std::string&)>& validator,
    const std::string& field_name = "Input",
    const std::string& prompt_msg = "Enter Input: ",
    const bool confirm = false,
    const bool hide = false);

std::string PromptRepoName(
    const std::string& prompt_msg = " -> Enter Repository Name: ",
    const bool confirm = false);

std::string PromptPassword(
    const std::string& prompt_msg = " -> Enter Password: ",
    const bool confirm = false);

std::string PromptPath(const std::string& prompt_msg = " -> Enter Path: ");

std::string PromptLocalPath(
    const std::string& prompt_msg = " -> Enter Local Path: ");

std::string PromptMountPath(
    const std::string& prompt_msg = " -> Enter Mount Path (Ex. /mnt/path): ");

std::string PromptSftpPath(const std::string& prompt_msg =
                               " -> Enter SFTP Path (Ex. user@host:/path): ");

std::string PromptIpAddress(
    const std::string& prompt_msg = " -> Enter IP Address: ");

}  // namespace Prompter

#endif  // PROMPTER_H_
