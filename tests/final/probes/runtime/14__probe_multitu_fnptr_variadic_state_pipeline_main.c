#include <stdarg.h>
#include <stdio.h>

typedef unsigned (*MvsFn)(unsigned seed, const char* spec, ...);

void mvs_reset(void);
MvsFn mvs_pick(unsigned selector);
unsigned mvs_digest(void);

int main(void) {
    MvsFn f0;
    MvsFn f1;
    unsigned a;
    unsigned b;
    unsigned c;
    unsigned digest = 47u;

    mvs_reset();
    f0 = mvs_pick(0u);
    f1 = mvs_pick(1u);

    a = f0(13u, "iu", -5, 31u);
    b = f1(29u, "ui", 17u, -3);
    c = f0(41u, "iiu", -1, 7, 19u);

    digest = digest * 179u + a;
    digest = digest * 179u + b;
    digest = digest * 179u + c;

    printf("%u %u\n", digest, mvs_digest());
    return 0;
}
