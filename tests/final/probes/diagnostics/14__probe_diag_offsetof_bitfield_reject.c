#include <stddef.h>

struct Bits {
    int x : 3;
    int y;
};

size_t k = offsetof(struct Bits, x);

int main(void) {
    return (int)k;
}
