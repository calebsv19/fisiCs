unsigned table_part_a(void);

static const unsigned k_table_b[] = {1u, 3u, 9u};

unsigned table_checksum(void) {
    unsigned i = 0;
    unsigned sum = table_part_a();
    while (i < (unsigned)(sizeof(k_table_b) / sizeof(k_table_b[0]))) {
        sum ^= (k_table_b[i] + i);
        ++i;
    }
    return sum;
}
