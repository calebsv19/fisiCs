extern int printf(const char *, ...);

struct wave37_state {
    int code;
    int payload[3];
};

int main(void) {
    struct wave37_state outer[3] = {
        {2, {3, 5, 7}},
        {11, {13, 17, 19}},
        {23, {29, 31, 37}}
    };
    struct wave37_state *captured = &outer[1];
    struct wave37_state **relay = &captured;
    int relay_arrow_checksum;
    int relay_dereference_checksum;
    int relay_neighbor_checksum;
    int captured_size;
    int inner_size;
    int inner_checksum;

    {
        struct wave37_state {
            int padding[6];
            int marker;
        } inner = {{41, 43, 47, 53, 59, 61}, 71};

        relay_arrow_checksum = (*relay)->code * 2
            + (*relay)->payload[0] * 3;
        relay_dereference_checksum = (**relay).payload[1] * 5
            + (**relay).payload[2] * 7;
        relay_neighbor_checksum = ((*relay) - 1)->code * 11
            + ((*relay) + 1)->code * 13;
        captured_size = (int)sizeof(**relay);
        inner_size = (int)sizeof(struct wave37_state);
        inner_checksum = inner.padding[0] + inner.marker;
    }

    int total = relay_arrow_checksum + relay_dereference_checksum * 2
        + relay_neighbor_checksum * 3 + captured_size * 5
        + inner_checksum * 7;
    printf("%d %d %d\n", total, captured_size, inner_size);
    return 0;
}
