#include <iostream>
#include <string>

#include "init/init.h"

int main(int argc, char* argv[]) {
  if (argc != 2) {
    std::cerr << "Usage: " << argv[0] << " --cli | --gui" << std::endl;
    return 1;
  }

  std::string mode(argv[1]);
  if (mode == "--cli") {
    (void)RunCLI(argc, argv);
  } else if (mode == "--gui") {
    (void)RunGUI(argc, argv);
  } else {
    std::cerr << "Invalid argument: " << mode << std::endl;
    return EXIT_FAILURE;
  }

  return EXIT_SUCCESS;
}
