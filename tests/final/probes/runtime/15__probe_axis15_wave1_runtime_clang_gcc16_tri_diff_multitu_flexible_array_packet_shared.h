#ifndef AXIS15_FLEXIBLE_ARRAY_PACKET_SHARED_H
#define AXIS15_FLEXIBLE_ARRAY_PACKET_SHARED_H

#include <stddef.h>

struct axis15_packet {
    size_t length;
    unsigned char bytes[];
};

unsigned axis15_digest(const struct axis15_packet *packet, unsigned seed);

#endif
