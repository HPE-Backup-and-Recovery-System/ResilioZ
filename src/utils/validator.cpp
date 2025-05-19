#include "utils/validator.h"

#include <regex>

namespace Validator {

bool IsValidLocalPath(const std::string& path) {
  std::regex pattern(
      R"(^([A-Za-z]:\\|\.{1,2}\\|\.{1,2}/|\\|/)?([A-Za-z0-9._-]+[\\/])*([A-Za-z0-9._-]+)?$)");
  return std::regex_match(path, pattern);
}

bool IsValidSftpPath(const std::string& path) {
  // Format: sftp:user@host:/absolute/path
  std::regex pattern(R"(^(sftp|ssh):[\w.-]+@[\w.-]+:\/[^\s]+$)");
  return std::regex_match(path, pattern);
}

bool IsValidPassword(const std::string& password) {
  // Any Characters, Max = 64
  std::regex pattern(R"(^.{0,64}$)");
  return std::regex_match(password, pattern);
}

bool IsValidRepoName(const std::string& name) {
  // Only Alphanumeric, Dashes or Underscores, No Spaces
  std::regex pattern(R"(^[a-zA-Z0-9_-]+$)");
  return std::regex_match(name, pattern);
}

bool IsValidIpAddress(const std::string& ip) {
  // Basic IPv4 check
  std::regex pattern(
      R"(^((25[0-5]|2[0-4][0-9]|[01]?[0-9][0-9]?)\.){3}"
      R"(25[0-5]|2[0-4][0-9]|[01]?[0-9][0-9]?)$)");
  return std::regex_match(ip, pattern);
}

}  // namespace Validator
