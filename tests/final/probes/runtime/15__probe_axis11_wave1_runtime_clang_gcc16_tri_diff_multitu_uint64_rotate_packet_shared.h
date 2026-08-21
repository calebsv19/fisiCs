#ifndef AXIS11_UINT64_PACKET_SHARED_H
#define AXIS11_UINT64_PACKET_SHARED_H

#include <stdint.h>

struct axis11_packet {
    uint64_t value;
    uint32_t tag;
};

struct axis11_packet axis11_round(struct axis11_packet input, uint64_t seed,
                                  unsigned lane);

#endif
