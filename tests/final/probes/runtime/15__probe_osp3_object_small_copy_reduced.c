typedef unsigned long osp3_u64;

struct osp3_small_copy_state {
    osp3_u64 lanes[2];
};

osp3_u64 osp3_object_small_copy_reduced(
    struct osp3_small_copy_state *destination,
    const struct osp3_small_copy_state *source,
    osp3_u64 selector
) {
    *destination = *source;
    return destination->lanes[selector & 1UL];
}
