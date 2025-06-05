#include "utils/user_io.h"

#include <iostream>

std::string UserIO::GetHiddenInput() {
  std::string input;

  // TODO: Implement * or No Input Visible Feature...
  std::cin >> input;

  return input;
}
