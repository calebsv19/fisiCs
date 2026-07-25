typedef unsigned long osp3_u64;

osp3_u64 osp3_object_pointer_window(
    const osp3_u64 *begin,
    const osp3_u64 *end,
    osp3_u64 seed
) {
    const osp3_u64 *cursor = begin;
    while (cursor < end) {
        seed ^= *cursor;
        seed = (seed << 9) | (seed >> 55);
        cursor = cursor + 1;
    }
    return seed;
}
