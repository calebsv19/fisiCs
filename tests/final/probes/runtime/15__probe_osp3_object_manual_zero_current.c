typedef unsigned long osp3_u64;

struct osp3_manual_zero_state {
    osp3_u64 lanes[4];
};

osp3_u64 osp3_object_manual_zero_current(osp3_u64 seed) {
    struct osp3_manual_zero_state state;
    state.lanes[0] = 0;
    state.lanes[1] = 0;
    state.lanes[2] = 0;
    state.lanes[3] = 0;
    state.lanes[seed & 3UL] = seed;
    return state.lanes[0] ^ state.lanes[1] ^
           state.lanes[2] ^ state.lanes[3];
}
