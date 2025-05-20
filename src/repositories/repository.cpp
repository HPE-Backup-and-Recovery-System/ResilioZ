#include "repositories/repository.h"

#include <openssl/sha.h>

#include <iomanip>
#include <sstream>

std::string Repository::GetName() const { return name_; }

std::string Repository::GetPath() const { return path_; }

std::string Repository::GetHashedPassword() const {
  return GetHashedPassword(password_);
}

std::string Repository::GetHashedPassword(const std::string& password) {
  unsigned char hash[SHA256_DIGEST_LENGTH];
  SHA256(reinterpret_cast<const unsigned char*>(password.c_str()),
         password.size(), hash);

  std::ostringstream oss;
  for (int i = 0; i < SHA256_DIGEST_LENGTH; ++i) {
    oss << std::hex << std::setw(2) << std::setfill('0')
        << static_cast<int>(hash[i]);
  }

  return oss.str();
}
