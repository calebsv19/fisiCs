typedef unsigned long osp3_u64;

struct osp3_small_zero_state {
    osp3_u64 lanes[4];
};

osp3_u64 osp3_object_small_zero_strict(osp3_u64 seed) {
    struct osp3_small_zero_state state = {0};
    osp3_u64 i;
    state.lanes[seed & 3UL] = seed;
    for (i = 0; i < 4; ++i) {
        seed ^= state.lanes[i] + (i * 17UL);
    }
    return seed;
}
