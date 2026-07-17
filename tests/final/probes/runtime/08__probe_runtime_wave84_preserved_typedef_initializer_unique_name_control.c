extern int printf(const char *, ...);

typedef struct wave84_packet {
    unsigned int marker;
    unsigned int payload[2];
    unsigned int seal;
} wave84_outer_t;

static unsigned int wave84_checksum(const wave84_outer_t *value) {
    return value->marker * 3u + value->payload[0] * 5u
        + value->payload[1] * 7u + value->seal * 11u;
}

int main(void) {
    unsigned int rebuilt_checksum;
    unsigned int inner_checksum;
    unsigned int outer_size;
    unsigned int inner_size;

    {
        struct wave84_inner_packet {
            unsigned int padding[5];
            unsigned int inner_marker;
        } inner = {{13u, 17u, 19u, 23u, 29u}, 31u};

        wave84_outer_t rebuilt = {
            .marker = 3u,
            .payload = {[0] = 5u, [1] = 7u},
            .seal = 11u,
        };

        rebuilt_checksum = wave84_checksum(&rebuilt);
        inner_checksum = inner.padding[0] + inner.inner_marker;
        outer_size = (unsigned int)sizeof(rebuilt);
        inner_size = (unsigned int)sizeof(struct wave84_inner_packet);
    }

    printf("%u %u %u\n", rebuilt_checksum * 3u + inner_checksum * 5u,
           outer_size, inner_size);
    return 0;
}
