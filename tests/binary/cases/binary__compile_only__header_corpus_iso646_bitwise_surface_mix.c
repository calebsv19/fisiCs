#include <iso646.h>

unsigned wave18_iso646_bitwise_surface(unsigned lhs, unsigned rhs) {
    unsigned value = lhs bitand rhs;
    value = value bitor (lhs xor rhs);
    value and_eq 0xffu;
    value xor_eq (compl rhs) bitand 0x0fu;
    value or_eq 0x10u;
    return value;
}
