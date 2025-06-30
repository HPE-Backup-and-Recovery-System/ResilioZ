#ifndef PROMPTER_H_
#define PROMPTER_H_

#include <functional>
#include <string>

namespace Prompter {

std::string PromptUntilValid(
    const std::function<bool(const std::string&)>& validator,
    const std::string& field_name = "input",
    const std::string& field_prompt = "value", const bool confirm = false,
    const bool hide = false);

std::string PromptRepoName(const std::string& prompt_msg = "Repository Name",
                           const bool confirm = false);

std::string PromptPassword(const std::string& prompt_msg = "Password",
                           const bool confirm = false);

std::string PromptPath(const std::string& prompt_msg = "Path");

std::string PromptLocalPath(const std::string& prompt_msg = "Local Path");

std::string PromptMountPath(
    const std::string& prompt_msg = "Mount Path (Ex. /mnt/path)");

std::string PromptSftpPath(
    const std::string& prompt_msg = "SFTP Path (Ex. user@host:/path)");

std::string PromptNfsPath(
    const std::string& prompt_msg = "NFS Path (Ex. host:/path)");

std::string PromptIpAddress(const std::string& prompt_msg = "IP Address");

std::string PromptInput(const std::string& prompt_msg = "Input");

std::string PromptCronString(
    const std::string& prompt_msg = "Cron String (Ex. 0 0 12 * * ? - Every Day at 12:00:00)\n    Format: Second Minute Hour Day Month Weekday");

std::string PromptScheduleId(
    const std::string& prompt_msg = "Schedule ID (Ex. #4)");

}  // namespace Prompter

#endif  // PROMPTER_H_
