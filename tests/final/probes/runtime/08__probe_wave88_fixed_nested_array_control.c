#include <stddef.h>
#include <stdio.h>

struct fixed_cell {
    unsigned char tag;
    unsigned long value;
};

struct fixed_tail_control {
    unsigned char kind;
    struct fixed_cell cells[3];
};

int main(void) {
    struct fixed_tail_control tail = {0};
    unsigned long checksum = 0;
    unsigned int index;
    unsigned long stride;

    tail.kind = 7;
    for (index = 0; index < 3; ++index) {
        tail.cells[index].tag = (unsigned char)(index + 1U);
        tail.cells[index].value = 100UL * (index + 1U);
        checksum += tail.cells[index].tag + tail.cells[index].value;
    }
    stride = (unsigned long)((unsigned char *)&tail.cells[2] -
                             (unsigned char *)&tail.cells[1]);

    printf("%lu %lu %lu %lu %lu\n",
           (unsigned long)sizeof(struct fixed_tail_control),
           (unsigned long)offsetof(struct fixed_tail_control, cells),
           (unsigned long)sizeof(struct fixed_cell),
           stride,
           checksum);

    return (stride == sizeof(struct fixed_cell) && checksum == 606UL) ? 0 : 1;
}
