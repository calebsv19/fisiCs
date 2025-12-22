#include <stdint.h>

int main(void) {
    int32_t a = INT32_C(10);
    uint64_t b = UINT64_C(20);
    return (int)(a + (int32_t)(b >> 1));
}
