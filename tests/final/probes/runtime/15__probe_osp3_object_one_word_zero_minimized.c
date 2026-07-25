typedef unsigned long osp3_u64;

struct osp3_one_word_zero {
    osp3_u64 value;
};

osp3_u64 osp3_object_one_word_zero_minimized(osp3_u64 seed) {
    struct osp3_one_word_zero state = {0};
    state.value = state.value ^ seed;
    return state.value;
}
