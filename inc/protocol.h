#pragma once
#include <cstdint>

enum MessageType {
    START = 1,
    DATA = 2,
    END = 3,
    ACK = 4
};

struct Header {
    uint32_t type;
    uint32_t chunk_id;
    uint64_t data_size;
};
