#include "endian.h"

#include <stdint.h>

uint16_t endian16(uint16_t u16)
{
    uint16_t endian = 0;
    uint8_t* l = (uint8_t*)&(endian);
    uint8_t* r = (uint8_t*)&(u16);
    *(l + 0) = *(r + 1);
    *(l + 1) = *(r + 0);
    return endian;
}

uint32_t endian32(uint32_t u32)
{
    uint32_t endian = 0;
    uint8_t* l = (uint8_t*)&(endian);
    uint8_t* r = (uint8_t*)&(u32);
    *(l + 0) = *(r + 3);
    *(l + 1) = *(r + 2);
    *(l + 2) = *(r + 1);
    *(l + 3) = *(r + 0);
    return endian;
}

uint64_t endian64(uint64_t u64)
{
    uint64_t endian = 0;
    uint8_t* l = (uint8_t*)&(endian);
    uint8_t* r = (uint8_t*)&(u64);
    *(l + 0) = *(r + 7);
    *(l + 1) = *(r + 6);
    *(l + 2) = *(r + 5);
    *(l + 3) = *(r + 4);
    *(l + 4) = *(r + 3);
    *(l + 5) = *(r + 2);
    *(l + 6) = *(r + 1);
    *(l + 7) = *(r + 0);
    return endian;
}
