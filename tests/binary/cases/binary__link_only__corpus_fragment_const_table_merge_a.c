static const unsigned k_table_a[] = {2u, 4u, 8u, 16u};

unsigned table_part_a(void) {
    unsigned i = 0;
    unsigned sum = 0;
    while (i < (unsigned)(sizeof(k_table_a) / sizeof(k_table_a[0]))) {
        sum += k_table_a[i];
        ++i;
    }
    return sum;
}
