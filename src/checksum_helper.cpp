#include "checksum_helper.h"
#include <fstream>
#include <stdexcept>

std::array<unsigned char, SHA256_DIGEST_LENGTH>
compute_sha256(const std::string& filename)
{
    std::ifstream file(filename, std::ios::binary);
    if (!file)
        throw std::runtime_error("Could not open file: " + filename);

    std::array<unsigned char, SHA256_DIGEST_LENGTH> hash;
    SHA256_CTX sha256;
    SHA256_Init(&sha256);

    const size_t buffer_size = 64 * 1024;
    char buffer[buffer_size];

    while (file.good())
    {
        file.read(buffer, buffer_size);
        std::streamsize bytes_read = file.gcount();
        if (bytes_read > 0)
            SHA256_Update(&sha256, buffer, bytes_read);
    }
    SHA256_Final(hash.data(), &sha256);
    return hash;
}