#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>

struct byte_tail {
    unsigned short count;
    unsigned char data[];
};

typedef char byte_tail_prefix_has_no_trailing_padding[
    sizeof(struct byte_tail) == offsetof(struct byte_tail, data) ? 1 : -1
];

int main(void) {
    const unsigned short count = 5;
    struct byte_tail *tail =
        (struct byte_tail *)malloc(sizeof(struct byte_tail) + count * sizeof(tail->data[0]));
    unsigned int checksum = 0;
    unsigned short index;

    if (tail == NULL) {
        return 2;
    }

    tail->count = count;
    for (index = 0; index < tail->count; ++index) {
        tail->data[index] = (unsigned char)(3U * index + 5U);
        checksum += tail->data[index];
    }

    printf("%lu %lu %u\n",
           (unsigned long)sizeof(struct byte_tail),
           (unsigned long)offsetof(struct byte_tail, data),
           checksum);
    free(tail);

    return (sizeof(struct byte_tail) == offsetof(struct byte_tail, data) && checksum == 55U)
               ? 0
               : 1;
}
