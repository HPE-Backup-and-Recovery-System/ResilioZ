#include "utils/time_util.h"

#include <chrono>
#include <ctime>
#include <iomanip>
#include <sstream>

std::string TimeUtil::GetCurrentTimestamp() {
  auto now = std::chrono::system_clock::now();
  std::time_t t = std::chrono::system_clock::to_time_t(now);
  std::ostringstream oss;
  oss << std::put_time(std::gmtime(&t), "%Y-%m-%dT%H:%M:%SZ");
  return oss.str();
}
