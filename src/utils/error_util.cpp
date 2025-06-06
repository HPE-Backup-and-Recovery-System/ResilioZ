#include "utils/error_util.h"

#include <exception>
#include <iostream>
#include <sstream>

#include "utils/logger.h"

void ErrorUtil::ThrowError(const std::string& message) {
  throw std::runtime_error(message);
}

void ErrorUtil::ThrowNested(const std::string& message) {
  std::throw_with_nested(std::runtime_error(message));
}

void ErrorUtil::LogException(const std::exception& e,
                             const std::string& context) {
  ErrorUtil::LogExceptionTerminal(e, context);
  ErrorUtil::LogExceptionSystem(e, context);
}

void ErrorUtil::LogExceptionTerminal(const std::exception& e,
                                     const std::string& context) {
  std::ostringstream oss;
  if (!context.empty()) oss << context << ":\n";
  ErrorUtil::LogExceptionChainToStream(e, oss, 0, true);
  Logger::TerminalLog(oss.str(), LogLevel::ERROR);
}

void ErrorUtil::LogExceptionSystem(const std::exception& e,
                                   const std::string& context) {
  std::ostringstream oss;
  if (!context.empty()) oss << context << " | ";
  ErrorUtil::LogExceptionChainToStream(e, oss, 0, false);
  Logger::SystemLog(oss.str(), LogLevel::ERROR);
}

void ErrorUtil::LogExceptionChainToStream(const std::exception& e,
                                          std::ostringstream& oss, int depth,
                                          bool indent) {
  if (depth > 0 && indent) oss << std::string(depth * 2 - 1, ' ') << "- ";
  if (depth > 0 && !indent) oss << " | ";

  oss << e.what();
  if (indent) oss << "\n";

  try {
    std::rethrow_if_nested(e);
  } catch (const std::exception& nested) {
    ErrorUtil::LogExceptionChainToStream(nested, oss, depth + 1, indent);
  } catch (...) {
    if (indent) {
      oss << std::string((depth + 1) * 2, ' ')
          << "<Unknown Nested Exception>\n";
    } else {
      oss << "| <Unknown Nested Exception>";
    }
  }
}