extern int axis1_w2_extent_bridge_step(int idx, int salt);

int axis1_w2_extent_table[9] = {4, 9, 2, 15, 6, 13, 8, 5, 11};

int axis1_w2_extent_count(void) {
    return (int)(sizeof(axis1_w2_extent_table) / sizeof(axis1_w2_extent_table[0]));
}

int axis1_w2_extent_lane_mix(int idx, int salt) {
    int count = axis1_w2_extent_count();
    int slot = idx % count;
    int v = axis1_w2_extent_table[slot];
    int bridge = axis1_w2_extent_bridge_step((slot + count - 1) % count, salt + v);
    axis1_w2_extent_table[(slot + 4) % count] ^= (salt + bridge);
    return v * 7 + bridge - salt;
}
