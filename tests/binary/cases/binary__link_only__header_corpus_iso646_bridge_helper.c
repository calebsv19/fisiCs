#include <iso646.h>

unsigned wave18_iso646_bridge_fold(unsigned lhs, unsigned rhs) {
    unsigned merged = (lhs bitand 0xf0u) bitor (rhs bitand 0x0fu);
    merged xor_eq (lhs xor rhs) bitand 0x3cu;
    return merged;
}
