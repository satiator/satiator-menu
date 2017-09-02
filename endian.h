#include <stdint.h>

static uint16_t htole16(uint16_t in) {
    return (in<<8) | (in>>8);
}

static uint32_t htole32(uint32_t in) {
    return ((in & 0x000000ff) << 24) |
           ((in & 0x0000ff00) << 8) |
           ((in & 0x00ff0000) >> 8) |
           ((in & 0xff000000) >> 24);
}
