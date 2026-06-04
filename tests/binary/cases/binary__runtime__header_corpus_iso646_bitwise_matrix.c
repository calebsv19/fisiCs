#include <iso646.h>
#include <stdio.h>

static unsigned wave18_iso646_mix(unsigned lhs, unsigned rhs) {
    unsigned value = lhs bitand 0xf0u;
    value or_eq rhs bitand 0x0fu;
    value xor_eq (lhs xor rhs) bitand 0x33u;
    value and_eq compl 0x40u;
    return value;
}

int main(void) {
    unsigned first = wave18_iso646_mix(0xb6u, 0x29u);
    unsigned second = wave18_iso646_mix(0x5au, 0xc3u);
    unsigned total = (first << 8) bitor second;

    printf("iso646-bitwise total=0x%x\n", total);
    return total == 0xaa02u ? 0 : 1;
}
