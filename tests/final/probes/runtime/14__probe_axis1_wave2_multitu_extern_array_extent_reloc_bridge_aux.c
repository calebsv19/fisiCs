extern int axis1_w2_extent_table[];
extern int axis1_w2_extent_count(void);

int axis1_w2_extent_bridge_step(int idx, int salt) {
    int count = axis1_w2_extent_count();
    int slot = (idx + salt) % count;
    if (slot < 0) {
        slot += count;
    }
    axis1_w2_extent_table[slot] += (salt & 3) + idx;
    return axis1_w2_extent_table[slot] * 5 + slot + salt;
}
