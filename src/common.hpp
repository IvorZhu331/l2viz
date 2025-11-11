#pragma once
#include <cstdint>

#pragma pack(push, 1)
struct L2Event {
    uint64_t ts_ns;
    double   price;
    uint32_t size;
    uint8_t  side;
    uint8_t  rtype;
};
#pragma pack(pop)
