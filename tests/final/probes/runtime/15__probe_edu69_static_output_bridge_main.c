#include <stdio.h>

extern unsigned long edu69_static_output_bridge(unsigned char seed);

unsigned long edu69_static_output_sink(const unsigned char *bytes, unsigned int length) {
    unsigned long digest = 2166136261u;
    unsigned int index;

    for (index = 0; index < length; ++index) {
        digest ^= bytes[index];
        digest *= 16777619u;
    }
    return digest;
}

int main(void) {
    printf("edu69-static-output=%lu,%lu\n",
           edu69_static_output_bridge(0x69u),
           edu69_static_output_bridge(0x2au));
    return 0;
}
