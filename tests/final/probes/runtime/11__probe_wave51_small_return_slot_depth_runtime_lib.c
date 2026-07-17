struct wave51_slot_pair {
    int x;
    int y;
};

typedef struct wave51_slot_pair (*wave51_slot_fn)(struct wave51_slot_pair item, int salt);

static struct wave51_slot_pair wave51_slot_local_swap(struct wave51_slot_pair item, int salt) {
    struct wave51_slot_pair out;
    out.x = item.y + salt * 2;
    out.y = item.x - salt;
    return out;
}

static struct wave51_slot_pair wave51_slot_local_fold(struct wave51_slot_pair item, int salt) {
    struct wave51_slot_pair out;
    out.x = item.x + item.y + salt;
    out.y = item.x * 2 - item.y + salt;
    return out;
}

wave51_slot_fn wave51_small_return_slot_depth(int depth,
                                              wave51_slot_fn first,
                                              wave51_slot_fn second) {
    wave51_slot_fn table[4];
    table[0] = first;
    table[1] = wave51_slot_local_swap;
    table[2] = second;
    table[3] = wave51_slot_local_fold;
    return table[(depth < 0 ? -depth : depth) & 3];
}
