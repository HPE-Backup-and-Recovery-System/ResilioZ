#include "utils/input.h"

#include <iostream>

namespace input {

std::string GetHiddenInput() {
  std::string input;

  // TODO: Implement * or No Input Visible Feature...
  std::cin >> input;

  return input;
}

}  // namespace input
