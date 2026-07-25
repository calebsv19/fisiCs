typedef unsigned long osp3_u64;

struct osp3_large_zero_state {
    osp3_u64 lanes[64];
};

osp3_u64 osp3_object_large_zero_strict(osp3_u64 seed) {
    struct osp3_large_zero_state state = {0};
    osp3_u64 i;
    state.lanes[seed & 63UL] = seed;
    for (i = 0; i < 64; ++i) {
        seed ^= state.lanes[i] + (i * 17UL);
        seed = (seed << 7) | (seed >> 57);
    }
    return seed;
}
