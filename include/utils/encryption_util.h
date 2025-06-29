#ifndef ENCRYPTION_UTIL_H
#define ENCRYPTION_UTIL_H

#include <cstdint>
#include <string>
#include <vector>

namespace EncryptionUtil {

    std::vector<uint8_t> EncryptMetadata(const std::string& plaintext, const std::string& password);
    std::string DecryptMetadata(const std::vector<uint8_t>& encrypted_data, const std::string& password);
    bool IsEncrypted(const std::vector<uint8_t>& data);
    bool IsEncrypted(const std::string& data);

} // namespace MetadataEncryption

#endif // ENCRYPTION_UTIL_H 