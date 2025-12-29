#include <stdint.h>

union U {
    struct {
        uint8_t lo;
        uint8_t hi;
    } bytes;
    uint16_t whole;
};

static int sum_bytes(union U u) {
    return u.bytes.lo + u.bytes.hi;
}

int main(void) {
    union U u = { .whole = 0x1234 };
    return sum_bytes(u) ? 0 : 1;
}
