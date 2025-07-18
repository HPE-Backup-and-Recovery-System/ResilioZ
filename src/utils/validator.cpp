#include "utils/validator.h"

#include <algorithm>
#include "libcron/CronData.h"
#include <regex>

bool Validator::Any(const std::string& str) { return true; }

bool Validator::IsValidPath(const std::string& path) {
  return IsValidLocalPath(path) || IsValidSftpPath(path) || IsValidNfsPath(path);
}

bool Validator::IsValidLocalPath(const std::string& path) {
  std::regex pattern(
      R"(^([A-Za-z]:\\|\.{1,2}\\|\.{1,2}/|\\|/)?([A-Za-z0-9._-]+[\\/])*([A-Za-z0-9._-]+)?$)");
  return std::regex_match(path, pattern);
}

bool Validator::IsValidSftpPath(const std::string& path) {
  // Matches: user@hostname:/absolute/path
  std::regex pattern(R"(^(""|(".*?")|([\w.\-]+))@([\w.\-]+):\/[^\s]+$)");
  return std::regex_match(path, pattern);
}

bool Validator::IsValidNfsPath(const std::string& path) {
  // Matches: hostname (or) ip:/absolute/path
  // Accepts IPv4/Hostnames addresses and absolute paths
  
  std::regex pattern(R"(^([\w.-]+):\/[^\s]+$)");
  return std::regex_match(path, pattern);
}

bool Validator::IsValidPassword(const std::string& password) {
  // Any Characters, Max = 64
  std::regex pattern(R"(^.{0,64}$)");
  
  // Strict Password Validation
  // std::regex pattern(R"(^(?=.*[a-z])(?=.*[A-Z])(?=.*\d)(?=.*[^a-zA-Z\d]).{8,64}$)");
  return std::regex_match(password, pattern);
}

bool Validator::IsValidRepoName(const std::string& name) {
  // Only Alphanumeric, Dashes or Underscores, No Spaces
  std::regex pattern(R"(^[a-zA-Z0-9_-]+$)");
  return std::regex_match(name, pattern);
}

bool Validator::IsValidIpAddress(const std::string& ip) {
  // Standard IPv4 validation pattern that works for all valid IPs including private ranges
  std::regex pattern(R"(^(?:(?:25[0-5]|2[0-4][0-9]|[01]?[0-9][0-9]?)\.){3}(?:25[0-5]|2[0-4][0-9]|[01]?[0-9][0-9]?)$)");
  return std::regex_match(ip, pattern);
}

bool Validator::IsValidCronString(const std::string& cron_string){
  // To be changed!
  auto cron = libcron::CronData::create(cron_string);
  bool res = cron.is_valid();
  if (res){
    return true;
  }
  return false;
}

bool Validator::IsValidScheduleId(const std::string& schedule_id){
  std::regex pattern(R"(^#[1-9][0-9]*$)");
  return std::regex_match(schedule_id, pattern);
}