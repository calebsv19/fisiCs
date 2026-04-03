#include <stdio.h>

typedef struct {
    long long a;
    int b;
    int c;
} RetPack;

static int ret_i(int x) { return x + 2; }
static long long ret_ll(long long x) { return x * 3LL; }
static RetPack ret_pack(int x) {
    RetPack v;
    v.a = (long long)x * 10LL;
    v.b = x + 1;
    v.c = x + 2;
    return v;
}

int main(void) {
    RetPack p = ret_pack(6);
    long long value = (long long)ret_i(5) + ret_ll(4) + p.a + (long long)p.b + (long long)p.c;
    printf("%lld\n", value);
    return 0;
}
