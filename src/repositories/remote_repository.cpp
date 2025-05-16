#include "repositories/remote_repository.h"

#include <iostream>

RemoteRepository::RemoteRepository() {}

RemoteRepository::RemoteRepository(const std::string& ip,
                                   const std::string& username,
                                   const std::string& password,
                                   const std::string& name)
    : ip_(ip), username_(username), password_(password), name_(name) {}

bool RemoteRepository::Exists() const {
  std::cout << " *** Checking Existence via SSH NOT Implemented Yet! *** "
            << std::endl;
  return false;
}

void RemoteRepository::Initialize() {
  std::cout << " *** Remote Repository Setup via SSH NOT Implemented Yet! *** "
            << std::endl;
}

std::string RemoteRepository::GetName() const { return name_; }
