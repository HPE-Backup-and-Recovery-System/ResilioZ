#pragma once
#include <string>

// Encrypts the entire repository folder as a tar archive using AES-256-CBC.
// Returns true on success, false on failure.
bool EncryptRepository(const std::string& repo_path, const std::string& password);

// Decrypts the .tar.enc file to the specified output directory.
// Returns true on success, false on failure.
bool DecryptRepository(const std::string& encrypted_file, const std::string& password, const std::string& output_dir); 