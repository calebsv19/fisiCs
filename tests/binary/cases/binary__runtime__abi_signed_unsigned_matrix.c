#include <stdio.h>

static unsigned w16_score(int a, unsigned b, long long c, unsigned long long d) {
    unsigned out = 0;
    out += ((unsigned)a < b) ? 1u : 9u;
    out += (unsigned)(a + 20);
    out += b * 2u;
    out += (unsigned)(c + 30);
    out += (unsigned)(d - 5ull);
    return out;
}

int main(void) {
    printf("%u\n", w16_score(-7, 11u, -13ll, 17ull));
    return 0;
}
