#include <stdio.h>

unsigned int grouped_fold(const unsigned int *values, unsigned int count);
unsigned int streamed_fold(const unsigned int *values, unsigned int count);

int main(void) {
    static const unsigned int values[] = {
        0xfffffff1u, 0x00000023u, 0x80000011u, 0x7ffffffdu,
        0x10203040u, 0xf0e0d0c0u, 0x00000101u
    };
    unsigned int a = grouped_fold(values, 7u);
    unsigned int b = streamed_fold(values, 7u);
    printf("%u %u %u\n", a, b, a == b);
    return 0;
}
