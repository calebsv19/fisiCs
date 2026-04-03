#include <stddef.h>

typedef struct {
    unsigned lane;
    unsigned value;
} Packet;

typedef struct {
    Packet ring[32];
    unsigned head;
} Pipeline;

static unsigned rotl(unsigned v, unsigned s) {
    return (v << (s & 31u)) | (v >> ((32u - s) & 31u));
}

unsigned pipe_push(Pipeline *p, Packet pkt) {
    unsigned slot = p->head & 31u;
    unsigned mix = rotl(pkt.value ^ pkt.lane, slot);
    p->ring[slot] = pkt;
    p->head++;
    return mix;
}

unsigned pipe_digest(const Pipeline *p) {
    unsigned i;
    unsigned acc = 0x9e3779b9u;
    for (i = 0; i < 32u; ++i) {
        acc ^= rotl(p->ring[i].value + p->ring[i].lane * 17u, i);
        acc *= 16777619u;
    }
    return acc ^ p->head;
}
