#ifndef VALIDATOR_H_
#define VALIDATOR_H_

#include <string>

namespace Validator {

bool IsValidPath(const std::string& path);
bool IsValidLocalPath(const std::string& path);
bool IsValidMountPath(const std::string& path);
bool IsValidSftpPath(const std::string& path);
bool IsValidPassword(const std::string& password);
bool IsValidRepoName(const std::string& name);
bool IsValidIpAddress(const std::string& ip);
bool isValidCronString(const std::string& cron_string);
bool isValidBackupType(const std::string& cron_string);
bool isValidScheduleId(const std::string& cron_string);

}  // namespace Validator

#endif  // VALIDATOR_H_
