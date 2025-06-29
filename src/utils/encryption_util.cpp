#include "utils/encryption_util.h"

#include <openssl/evp.h>
#include <openssl/rand.h>
#include <openssl/sha.h>

#include <algorithm>
#include <iomanip>
#include <nlohmann/json.hpp>
#include <sstream>

#include "utils/error_util.h"
#include "utils/logger.h"

namespace EncryptionUtil {

// Magic bytes to identify encrypted metadata
const std::vector<uint8_t> ENCRYPTION_MAGIC = {0x42, 0x41, 0x43, 0x4B, 0x55, 0x50, 0x45, 0x4E, 0x43}; // "BACKUPENC"
const size_t SALT_SIZE = 32;
const size_t IV_SIZE = 16;
const size_t MAGIC_SIZE = ENCRYPTION_MAGIC.size();

std::vector<uint8_t> EncryptMetadata(const std::string& plaintext, const std::string& password) {
    if (password.empty()) {
        // If password is empty, return the plaintext as-is
        return std::vector<uint8_t>(plaintext.begin(), plaintext.end());
    }

    try {
        // Generate random salt and IV
        std::vector<uint8_t> salt(SALT_SIZE);
        std::vector<uint8_t> iv(IV_SIZE);
        
        if (RAND_bytes(salt.data(), SALT_SIZE) != 1) {
            Logger::Log("Failed to generate random salt", LogLevel::ERROR);
            return std::vector<uint8_t>();
        }
        
        if (RAND_bytes(iv.data(), IV_SIZE) != 1) {
            Logger::Log("Failed to generate random IV", LogLevel::ERROR);
            return std::vector<uint8_t>();
        }

        // Derive key from password using PBKDF2
        std::vector<uint8_t> key(32); // 256-bit key for AES-256
        if (PKCS5_PBKDF2_HMAC(password.c_str(), password.length(),
                              salt.data(), salt.size(),
                              10000, // iterations
                              EVP_sha256(),
                              key.size(), key.data()) != 1) {
            Logger::Log("Failed to derive key from password", LogLevel::ERROR);
            return std::vector<uint8_t>();
        }

        // Initialize encryption context
        EVP_CIPHER_CTX* ctx = EVP_CIPHER_CTX_new();
        if (!ctx) {
            Logger::Log("Failed to create encryption context", LogLevel::ERROR);
            return std::vector<uint8_t>();
        }

        if (EVP_EncryptInit_ex(ctx, EVP_aes_256_cbc(), nullptr, key.data(), iv.data()) != 1) {
            Logger::Log("Failed to initialize encryption", LogLevel::ERROR);
            EVP_CIPHER_CTX_free(ctx);
            return std::vector<uint8_t>();
        }

        // Prepare output buffer
        std::vector<uint8_t> output;
        output.reserve(MAGIC_SIZE + SALT_SIZE + IV_SIZE + plaintext.length() + EVP_MAX_BLOCK_LENGTH);

        // Add magic bytes
        output.insert(output.end(), ENCRYPTION_MAGIC.begin(), ENCRYPTION_MAGIC.end());
        
        // Add salt
        output.insert(output.end(), salt.begin(), salt.end());
        
        // Add IV
        output.insert(output.end(), iv.begin(), iv.end());

        // Encrypt the data
        int out_len;
        std::vector<uint8_t> ciphertext(plaintext.length() + EVP_MAX_BLOCK_LENGTH);
        
        if (EVP_EncryptUpdate(ctx, ciphertext.data(), &out_len, 
                             reinterpret_cast<const uint8_t*>(plaintext.c_str()), 
                             plaintext.length()) != 1) {
            Logger::Log("Failed to encrypt data", LogLevel::ERROR);
            EVP_CIPHER_CTX_free(ctx);
            return std::vector<uint8_t>();
        }

        output.insert(output.end(), ciphertext.begin(), ciphertext.begin() + out_len);

        // Finalize encryption
        int final_len;
        if (EVP_EncryptFinal_ex(ctx, ciphertext.data(), &final_len) != 1) {
            Logger::Log("Failed to finalize encryption", LogLevel::ERROR);
            EVP_CIPHER_CTX_free(ctx);
            return std::vector<uint8_t>();
        }

        output.insert(output.end(), ciphertext.begin(), ciphertext.begin() + final_len);

        EVP_CIPHER_CTX_free(ctx);
        return output;

    } catch (const std::exception& e) {
        Logger::Log("Encryption failed: " + std::string(e.what()), LogLevel::ERROR);
        return std::vector<uint8_t>();
    }
}

std::string DecryptMetadata(const std::vector<uint8_t>& encrypted_data, const std::string& password) {
    if (password.empty()) {
        // If password is empty, treat data as plaintext
        return std::string(encrypted_data.begin(), encrypted_data.end());
    }

    try {
        // Check if data is encrypted
        if (!IsEncrypted(encrypted_data)) {
            // Data is not encrypted, check if it's valid JSON
            std::string data_str(encrypted_data.begin(), encrypted_data.end());
            
            // Try to parse as JSON to validate it's not corrupted
            try {
                nlohmann::json::parse(data_str);
                return data_str; // Return as plaintext if it's valid JSON
            } catch (...) {
                // If it's not valid JSON and not encrypted, it might be corrupted
                Logger::Log("Data appears to be neither encrypted nor valid JSON", LogLevel::ERROR);
                return "";
            }
        }

        // Extract components
        if (encrypted_data.size() < MAGIC_SIZE + SALT_SIZE + IV_SIZE) {
            Logger::Log("Encrypted data too short", LogLevel::ERROR);
            return "";
        }

        size_t offset = MAGIC_SIZE;
        std::vector<uint8_t> salt(encrypted_data.begin() + offset, 
                                 encrypted_data.begin() + offset + SALT_SIZE);
        offset += SALT_SIZE;
        
        std::vector<uint8_t> iv(encrypted_data.begin() + offset, 
                               encrypted_data.begin() + offset + IV_SIZE);
        offset += IV_SIZE;

        std::vector<uint8_t> ciphertext(encrypted_data.begin() + offset, encrypted_data.end());

        // Derive key from password using PBKDF2
        std::vector<uint8_t> key(32); // 256-bit key for AES-256
        if (PKCS5_PBKDF2_HMAC(password.c_str(), password.length(),
                              salt.data(), salt.size(),
                              10000, // iterations
                              EVP_sha256(),
                              key.size(), key.data()) != 1) {
            Logger::Log("Failed to derive key from password", LogLevel::ERROR);
            return "";
        }

        // Initialize decryption context
        EVP_CIPHER_CTX* ctx = EVP_CIPHER_CTX_new();
        if (!ctx) {
            Logger::Log("Failed to create decryption context", LogLevel::ERROR);
            return "";
        }

        if (EVP_DecryptInit_ex(ctx, EVP_aes_256_cbc(), nullptr, key.data(), iv.data()) != 1) {
            Logger::Log("Failed to initialize decryption", LogLevel::ERROR);
            EVP_CIPHER_CTX_free(ctx);
            return "";
        }

        // Decrypt the data
        int out_len;
        std::vector<uint8_t> plaintext(ciphertext.size());
        
        if (EVP_DecryptUpdate(ctx, plaintext.data(), &out_len, 
                             ciphertext.data(), ciphertext.size()) != 1) {
            Logger::Log("Failed to decrypt data", LogLevel::ERROR);
            EVP_CIPHER_CTX_free(ctx);
            return "";
        }

        // Finalize decryption
        int final_len;
        if (EVP_DecryptFinal_ex(ctx, plaintext.data() + out_len, &final_len) != 1) {
            Logger::Log("Failed to finalize decryption", LogLevel::ERROR);
            EVP_CIPHER_CTX_free(ctx);
            return "";
        }

        EVP_CIPHER_CTX_free(ctx);
        
        // Convert to string
        return std::string(plaintext.begin(), plaintext.begin() + out_len + final_len);

    } catch (const std::exception& e) {
        Logger::Log("Decryption failed: " + std::string(e.what()), LogLevel::ERROR);
        return "";
    }
}

bool IsEncrypted(const std::vector<uint8_t>& data) {
    if (data.size() < MAGIC_SIZE) {
        return false;
    }
    
    return std::equal(ENCRYPTION_MAGIC.begin(), ENCRYPTION_MAGIC.end(), data.begin());
}

bool IsEncrypted(const std::string& data) {
    std::vector<uint8_t> data_vec(data.begin(), data.end());
    return IsEncrypted(data_vec);
}

} // namespace EncryptionUtil