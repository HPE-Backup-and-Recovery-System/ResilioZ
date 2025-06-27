#include "utils/encryption_utils.h"
#include "utils/logger.h"
#include <cstdlib>
#include <sstream>
#include <filesystem>

namespace fs = std::filesystem;

bool EncryptRepository(const std::string& repo_path, const std::string& password) {
    std::string tar_file = repo_path + ".tar";
    std::string enc_file = repo_path + ".tar.enc";
    std::ostringstream tar_cmd;
    tar_cmd << "tar -cf '" << tar_file << "' -C '" << fs::path(repo_path).parent_path().string() << "' '" << fs::path(repo_path).filename().string() << "'";
    if (system(tar_cmd.str().c_str()) != 0) {
        Logger::Log("Failed to create tar archive for encryption: " + tar_file, LogLevel::ERROR);
        return false;
    }
    std::ostringstream enc_cmd;
    enc_cmd << "openssl enc -aes-256-cbc -pbkdf2 -salt -in '" << tar_file << "' -out '" << enc_file << "' -pass pass:" << password;
    if (system(enc_cmd.str().c_str()) != 0) {
        Logger::Log("Failed to encrypt tar archive: " + tar_file, LogLevel::ERROR);
        fs::remove(tar_file);
        return false;
    }
    // Remove the original tar and repo directory
    fs::remove(tar_file);
    fs::remove_all(repo_path);
    return true;
}

bool DecryptRepository(const std::string& encrypted_file, const std::string& password, const std::string& output_dir) {
    std::string tar_file = output_dir + ".tar";
    std::ostringstream dec_cmd;
    dec_cmd << "openssl enc -d -aes-256-cbc -pbkdf2 -in '" << encrypted_file << "' -out '" << tar_file << "' -pass pass:" << password;
    if (system(dec_cmd.str().c_str()) != 0) {
        Logger::Log("Failed to decrypt encrypted repository: " + encrypted_file, LogLevel::ERROR);
        return false;
    }
    std::ostringstream untar_cmd;
    untar_cmd << "tar -xf '" << tar_file << "' -C '" << fs::path(output_dir).parent_path().string() << "'";
    if (system(untar_cmd.str().c_str()) != 0) {
        Logger::Log("Failed to extract decrypted tar: " + tar_file, LogLevel::ERROR);
        fs::remove(tar_file);
        return false;
    }
    fs::remove(tar_file);
    return true;
} 