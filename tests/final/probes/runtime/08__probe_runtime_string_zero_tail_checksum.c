#include <stdio.h>

static unsigned checksum(const char *buf, int count) {
    unsigned acc = 0;

    for (int i = 0; i < count; ++i) {
        acc = acc * 131u + (unsigned char)buf[i];
    }
    return acc;
}

int main(void) {
    char label[8] = "ion";
    char exact[3] = {'a', 'b', 'c'};

    printf(
        "%u %u %u\n",
        checksum(label, 8),
        checksum(exact, 3),
        (unsigned char)label[3] + (unsigned char)label[7]
    );
    return 0;
}
