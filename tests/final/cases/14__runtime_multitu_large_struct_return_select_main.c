#include <stdio.h>

struct HugeState {
    long long lane[6];
    int tag;
    double scale;
};

struct HugeState abi_huge_make(int seed, long long base);
struct HugeState abi_huge_twist(struct HugeState in, int k);
struct HugeState abi_huge_blend(struct HugeState a, struct HugeState b, int lane);
long long abi_huge_score(struct HugeState in);

int main(void) {
    struct HugeState a = abi_huge_make(3, 1000LL);
    struct HugeState b = abi_huge_make(7, -250LL);
    struct HugeState x = abi_huge_twist(a, 4);
    struct HugeState y = abi_huge_blend(x, b, 2);
    struct HugeState z = (y.tag & 1) ? abi_huge_blend(y, a, 1) : abi_huge_blend(b, y, 3);

    long long s1 = abi_huge_score(y);
    long long s2 = abi_huge_score(z);
    printf("%lld %lld\n", s1, s2);
    return 0;
}
