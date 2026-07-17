extern int printf(const char *, ...);

struct wave36_packet {
    int key;
    int payload[2];
};

int main(void) {
    struct wave36_packet outer[3] = {
        {2, {3, 5}},
        {7, {11, 13}},
        {17, {19, 23}}
    };
    struct wave36_packet *captured = &outer[1];
    int arrow_checksum;
    int dereference_checksum;
    int neighbor_checksum;
    int captured_size;
    int inner_size;
    int inner_checksum;

    {
        struct wave36_inner_packet {
            int padding[5];
            int marker;
        } inner = {{29, 31, 37, 41, 43}, 47};

        arrow_checksum = captured->key * 2 + captured->payload[0] * 3;
        dereference_checksum = (*captured).payload[1] * 5
            + (*captured).key * 7;
        neighbor_checksum = (captured - 1)->key * 11
            + (captured + 1)->key * 13;
        captured_size = (int)sizeof(*captured);
        inner_size = (int)sizeof(struct wave36_inner_packet);
        inner_checksum = inner.padding[0] + inner.marker;
    }

    int total = arrow_checksum + dereference_checksum * 2
        + neighbor_checksum * 3 + captured_size * 5 + inner_checksum * 7;
    printf("%d %d %d\n", total, captured_size, inner_size);
    return 0;
}
