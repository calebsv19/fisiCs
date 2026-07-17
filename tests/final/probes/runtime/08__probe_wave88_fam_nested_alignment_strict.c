#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>

struct aligned_cell {
    unsigned char tag;
    unsigned long value;
};

union long_alignment_unit {
    unsigned char byte;
    unsigned long value;
};

struct aligned_tail {
    unsigned char kind;
    struct aligned_cell cells[];
};

typedef char aligned_tail_starts_at_complete_prefix[
    sizeof(struct aligned_tail) == offsetof(struct aligned_tail, cells) ? 1 : -1
];
typedef char aligned_tail_respects_nested_element_alignment[
    offsetof(struct aligned_tail, cells) % sizeof(union long_alignment_unit) == 0 ? 1 : -1
];

int main(void) {
    const unsigned int count = 3;
    struct aligned_tail *tail = (struct aligned_tail *)malloc(
        sizeof(struct aligned_tail) + count * sizeof(struct aligned_cell));
    unsigned long checksum = 0;
    unsigned int index;
    unsigned long stride;

    if (tail == NULL) {
        return 2;
    }

    tail->kind = 7;
    for (index = 0; index < count; ++index) {
        tail->cells[index].tag = (unsigned char)(index + 1U);
        tail->cells[index].value = 100UL * (index + 1U);
        checksum += tail->cells[index].tag + tail->cells[index].value;
    }
    stride = (unsigned long)((unsigned char *)&tail->cells[2] -
                             (unsigned char *)&tail->cells[1]);

    printf("%lu %lu %lu %lu %lu\n",
           (unsigned long)sizeof(struct aligned_tail),
           (unsigned long)offsetof(struct aligned_tail, cells),
           (unsigned long)sizeof(struct aligned_cell),
           stride,
           checksum);
    free(tail);

    return (sizeof(struct aligned_tail) == offsetof(struct aligned_tail, cells) &&
            stride == sizeof(struct aligned_cell) && checksum == 606UL)
               ? 0
               : 1;
}
