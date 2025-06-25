#ifndef APP_NAME
#define APP_NAME "ResilioZ"
#endif  // APP_NAME

#include "utils/setup.h"

#include <pwd.h>
#include <unistd.h>

#include <string>

#include "utils/error_util.h"

std::string Setup::GetAppDataPath() {
  try {
    const char* sudo_user = getenv("SUDO_USER");
    struct passwd* pw = nullptr;

    if (sudo_user) {
      pw = getpwnam(sudo_user);
    }

    if (!pw) {
      uid_t uid = getuid();
      pw = getpwuid(uid);
    }

    if (!pw || !pw->pw_dir) {
      ErrorUtil::ThrowError("Failed to resolve user's home directory");
    }

    return std::string(pw->pw_dir) + "/" + APP_NAME;
  } catch (...) {
    ErrorUtil::ThrowNested("Data path could not be resolved");
  }
}
