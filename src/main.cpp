#include "ui_handler.h"
#include <iostream>

int main() {
    try {
        const std::string backup_path = "/home/centronox/Documents/backups";
        hpe::backup::UIHandler ui(backup_path);
        ui.run();
    } catch (const std::exception& e) {
        std::cerr << "Fatal error: " << e.what() << std::endl;
        return 1;
    }

    return 0;
}