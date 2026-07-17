extern int printf(const char *, ...);

struct wave35_record {
    int left;
    int right[2];
};

int main(void) {
    struct wave35_record outer = {11, {17, 23}};
    struct wave35_record *outer_pointer = &outer;
    int inner_checksum;
    int outer_checksum_from_inner;
    int outer_size_from_inner;
    int inner_size;
    int inner_first;

    {
        struct wave35_record {
            int values[4];
        } inner = {{3, 5, 7, 9}};

        inner_checksum = inner.values[0] * 2 + inner.values[1] * 3
            + inner.values[2] * 5 + inner.values[3] * 7;
        outer_checksum_from_inner = outer.left * 11 + outer.right[0] * 13
            + outer_pointer->right[1] * 17;
        outer_size_from_inner = (int)sizeof(outer)
            + (int)sizeof(*outer_pointer);
        inner_size = (int)sizeof(struct wave35_record);
        inner_first = inner.values[0];
    }

    outer.left += inner_first;
    int outer_checksum_after_scope = outer.left * 19 + outer.right[0] * 23
        + outer_pointer->right[1] * 29;
    int total = inner_checksum + outer_checksum_from_inner * 2
        + outer_checksum_after_scope * 3;

    printf("%d %d %d\n", total, outer_size_from_inner, inner_size);
    return 0;
}
