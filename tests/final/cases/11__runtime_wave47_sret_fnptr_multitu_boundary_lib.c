typedef struct Big47D {
    unsigned long long slots[6];
} Big47D;

typedef Big47D (*BigMaker47D)(unsigned long long, unsigned long long);

static Big47D make_linear47(unsigned long long seed, unsigned long long step) {
    Big47D big;
    big.slots[0] = seed + step;
    big.slots[1] = seed + step * 2ULL;
    big.slots[2] = seed + step * 3ULL;
    big.slots[3] = seed + step * 4ULL;
    big.slots[4] = seed + step * 5ULL;
    big.slots[5] = seed + step * 6ULL;
    return big;
}

static Big47D make_shifted47(unsigned long long seed, unsigned long long step) {
    Big47D big;
    big.slots[0] = seed - step;
    big.slots[1] = seed + step;
    big.slots[2] = seed + step + 1ULL;
    big.slots[3] = seed + step + 2ULL;
    big.slots[4] = seed + step + 3ULL;
    big.slots[5] = seed + step + 4ULL;
    return big;
}

BigMaker47D wave47_select_big_maker(int route) {
    return (route & 1) ? make_shifted47 : make_linear47;
}

Big47D wave47_big_boundary_call(BigMaker47D maker, unsigned long long seed, unsigned long long step) {
    return maker(seed, step);
}
