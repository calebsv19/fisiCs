#include <stddef.h>
#include <stdio.h>

int main(void) {
    long long v[18];
    for (int i = 0; i < 18; ++i) {
        v[i] = (long long)(i * 31 - 7);
    }

    long long *base = &v[2];
    long long *end = &v[16];
    ptrdiff_t span = end - base;

    int signed_step = -3;
    unsigned int unsigned_step = 5u;

    long long *p = base + (ptrdiff_t)unsigned_step;
    long long *q = end + (ptrdiff_t)signed_step;

    unsigned char *pb = (unsigned char *)p;
    unsigned char *qb = (unsigned char *)q;
    ptrdiff_t byte_delta = qb - pb;

    long long pick = p[1] + q[-2];
    long long elem_delta = (long long)(byte_delta / (ptrdiff_t)sizeof(long long));

    printf("%lld %lld %lld\n", pick, (long long)span, elem_delta);
    return 0;
}
