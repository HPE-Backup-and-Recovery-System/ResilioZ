#ifndef ERROR_UTIL_H
#define ERROR_UTIL_H

#include <string>

namespace ErrorUtil {

[[noreturn]] void ThrowError(const std::string& message);

[[noreturn]] void ThrowNested(const std::string& message);

void LogException(const std::exception& e, const std::string& context);

void LogExceptionTerminal(const std::exception& e, const std::string& context);

void LogExceptionSystem(const std::exception& e, const std::string& context);

void LogExceptionChainToStream(const std::exception& e, std::ostringstream& oss,
                               int depth, bool indent);

}  // namespace ErrorUtil

#endif  // ERROR_UTIL_H
