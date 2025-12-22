#include <limits.h>

int main(void) {
    int m = INT_MAX;
    unsigned u = UINT_MAX;
    return (m > 0 && u > 0 && CHAR_BIT == 8) ? 0 : 1;
}
