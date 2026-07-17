// Check alignas/_Alignas propagate to IR alignment.
#include <stdalign.h>

alignas(128) int g_aligned = 3;

alignas(32) struct A {
    int x;
};

union AlignedUnion {
    alignas(32) int value;
    unsigned char bytes[32];
};

static union AlignedUnion g_union = { .value = 5 };

int main(void) {
    alignas(16) struct A a = { .x = g_aligned };
    alignas(32) int local = a.x;
    union AlignedUnion u = { .bytes = {7} };
    return local + a.x + g_union.value + u.bytes[0];
}
