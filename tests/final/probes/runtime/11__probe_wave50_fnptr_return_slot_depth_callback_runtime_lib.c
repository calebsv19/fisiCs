struct wave50_slot_cell {
    int value[3];
    int tag;
};

typedef struct wave50_slot_cell (*wave50_slot_transform_fn)(struct wave50_slot_cell item, int bias);

static struct wave50_slot_cell wave50_slot_scale(struct wave50_slot_cell item, int bias) {
    struct wave50_slot_cell out;
    int i;
    for (i = 0; i < 3; i++) {
        out.value[i] = item.value[i] * (bias + i + 1) + item.tag;
    }
    out.tag = item.tag + bias + 11;
    return out;
}

static struct wave50_slot_cell wave50_slot_shift(struct wave50_slot_cell item, int bias) {
    struct wave50_slot_cell out;
    int i;
    for (i = 0; i < 3; i++) {
        out.value[i] = item.value[2 - i] + bias * (i + 2);
    }
    out.tag = item.tag - bias + 7;
    return out;
}

static wave50_slot_transform_fn wave50_slot_choose(int depth, wave50_slot_transform_fn fallback) {
    if ((depth & 2) != 0) {
        return wave50_slot_scale;
    }
    return fallback;
}

wave50_slot_transform_fn wave50_fnptr_return_slot_depth_callback(int depth,
                                                                 wave50_slot_transform_fn first,
                                                                 wave50_slot_transform_fn fallback) {
    wave50_slot_transform_fn table[3];
    table[0] = first;
    table[1] = wave50_slot_shift;
    table[2] = wave50_slot_choose(depth, fallback);
    return table[depth % 3];
}
