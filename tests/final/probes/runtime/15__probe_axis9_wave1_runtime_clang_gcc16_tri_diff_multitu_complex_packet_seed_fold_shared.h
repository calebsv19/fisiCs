#ifndef AXIS9_COMPLEX_PACKET_SHARED_H
#define AXIS9_COMPLEX_PACKET_SHARED_H

#include <complex.h>

struct axis9_packet {
    double complex value;
    unsigned stamp;
};

struct axis9_packet axis9_step(struct axis9_packet input, unsigned seed);

#endif
