#include "backup/backup.hpp"
#include "backup/restore.hpp"
#include <iostream>
#include <string>
#include <filesystem>

void print_usage() {
    std::cout << "Usage: backup-ui [OPTIONS] [COMMAND]\n\n"
              << "Commands:\n"
              << "  backup    Backup files from input_dir to output_dir\n"
              << "  restore   Restore files from input_dir to output_dir\n"
              << "  list      List all backups in output_dir\n"
              << "  compare   Compare two backups\n"
              << "  help      Print this message\n\n"
              << "Options:\n"
              << "  -a, --average-size <SIZE>  Average chunk size in bytes (default: 8192)\n"
              << "  -t, --type <TYPE>         Backup type: full, incremental, differential (default: full)\n"
              << "  -h, --help                Print help message\n"
              << "  -V, --version             Print version\n";
}

int main(int argc, char* argv[]) {
    if (argc < 2) {
        print_usage();
        return 1;
    }

    std::string command = argv[1];
    size_t average_size = 1048576; // 1MB
    std::filesystem::path input_path;
    std::filesystem::path output_path;
    std::string backup_name;
    BackupType backup_type = BackupType::FULL;
    std::string remarks;

    // Parse command line arguments
    for (int i = 2; i < argc; i++) {
        std::string arg = argv[i];
        if (arg == "-h" || arg == "--help") {
            print_usage();
            return 0;
        } else if (arg == "-V" || arg == "--version") {
            std::cout << "backup-ui version 1.0\n";
            return 0;
        } else if (arg == "-a" || arg == "--average-size") {
            if (i + 1 < argc) {
                average_size = std::stoull(argv[++i]);
            }
        } else if (arg == "-t" || arg == "--type") {
            if (i + 1 < argc) {
                std::string type = argv[++i];
                if (type == "full") {
                    backup_type = BackupType::FULL;
                } else if (type == "incremental") {
                    backup_type = BackupType::INCREMENTAL;
                } else if (type == "differential") {
                    backup_type = BackupType::DIFFERENTIAL;
                } else {
                    std::cerr << "Error: Invalid backup type '" << type << "'\n";
                    return 1;
                }
            }
        } else if (arg == "--input-path") {
            if (i + 1 < argc) {
                input_path = argv[++i];
            }
        } else if (arg == "--output-path") {
            if (i + 1 < argc) {
                output_path = argv[++i];
            }
        }
        else if (arg == "--backup-name") {
            if (i + 1 < argc) {
                backup_name = argv[++i];
            }
        }
        else if (arg == "--remarks") {
            if (i + 1 < argc) {
                remarks = argv[++i];
            }
        }
    }

    try {
        if (command == "backup") {
            if (input_path.empty() || output_path.empty()) {
                std::cerr << "Error: Both input-path and output-path must be specified for backup\n";
                return 1;
            }
            Backup backup(input_path, output_path, backup_type, remarks, average_size);
            backup.backup_directory();
            std::cout << "Backup completed successfully\n";
        } else if (command == "restore") {
            if (input_path.empty() || output_path.empty()) {
                std::cerr << "Error: Both input-path and output-path must be specified for restore\n";
                return 1;
            }
            Restore restore(input_path, output_path, backup_name);
            restore.restore_all();
            std::cout << "Restore completed successfully\n";
        } else if (command == "list") {
            Backup backup(input_path, "", BackupType::FULL, "", average_size);
            backup.print_all_backup_details(input_path);

        } else if (command == "compare") {
            if (input_path.empty() || argc < 4) {
                std::cerr << "Error: input-path and two backup names must be specified for compare\n";
                return 1;
            }
            std::string backup1 = argv[2];
            std::string backup2 = argv[3];
            Backup backup(input_path,"", BackupType::FULL, "", average_size);
            backup.compare_backups(input_path, backup1, backup2);
        } else if (command == "help") {
            print_usage();
        } else {
            std::cerr << "Error: Unknown command '" << command << "'\n";
            print_usage();
            return 1;
        }
    } catch (const std::exception& e) {
        std::cerr << "Error: " << e.what() << "\n";
        return 1;
    }

    return 0;
} 