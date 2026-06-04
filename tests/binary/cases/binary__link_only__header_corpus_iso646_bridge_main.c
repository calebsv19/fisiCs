#include <iso646.h>

unsigned wave18_iso646_bridge_fold(unsigned lhs, unsigned rhs);

int main(void) {
    unsigned folded = wave18_iso646_bridge_fold(0x2du, 0x17u);
    return (folded not_eq 0x38u) ? 1 : 0;
}
