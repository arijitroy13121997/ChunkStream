#pragma once

#include <openssl/sha.h>
#include <array>
#include <string>

std::array<unsigned char, SHA256_DIGEST_LENGTH>
compute_sha256(const std::string &filename);