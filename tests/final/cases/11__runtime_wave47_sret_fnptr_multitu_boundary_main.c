#include <stdio.h>

typedef struct Big47D {
    unsigned long long slots[6];
} Big47D;

typedef Big47D (*BigMaker47D)(unsigned long long, unsigned long long);

BigMaker47D wave47_select_big_maker(int route);
Big47D wave47_big_boundary_call(BigMaker47D maker, unsigned long long seed, unsigned long long step);

int main(void) {
    BigMaker47D maker = wave47_select_big_maker(2);
    Big47D big = wave47_big_boundary_call(maker, 10ULL, 3ULL);
    unsigned long long checksum = big.slots[0] + big.slots[1] + big.slots[2] +
        big.slots[3] + big.slots[4] + big.slots[5];
    if (big.slots[0] != 13ULL) return 1;
    if (big.slots[5] != 28ULL) return 2;
    if (checksum != 123ULL) return 3;
    printf("%llu %llu %llu\n", checksum, big.slots[3], big.slots[5]);
    return 0;
}
