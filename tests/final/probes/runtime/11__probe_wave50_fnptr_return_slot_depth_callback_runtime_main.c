#include <stdio.h>

struct wave50_slot_cell {
    int value[3];
    int tag;
};

typedef struct wave50_slot_cell (*wave50_slot_transform_fn)(struct wave50_slot_cell item, int bias);

wave50_slot_transform_fn wave50_fnptr_return_slot_depth_callback(int depth,
                                                                 wave50_slot_transform_fn first,
                                                                 wave50_slot_transform_fn fallback);

static struct wave50_slot_cell wave50_slot_first(struct wave50_slot_cell item, int bias) {
    struct wave50_slot_cell out;
    int i;
    for (i = 0; i < 3; i++) {
        out.value[i] = item.value[i] + bias + 10 * i;
    }
    out.tag = item.tag + bias + 3;
    return out;
}

static struct wave50_slot_cell wave50_slot_fallback(struct wave50_slot_cell item, int bias) {
    struct wave50_slot_cell out;
    int i;
    for (i = 0; i < 3; i++) {
        out.value[i] = item.value[i] * (i + 2) - bias;
    }
    out.tag = item.tag + 29 - bias;
    return out;
}

int main(void) {
    struct wave50_slot_cell seed = {{4, 7, 9}, 6};
    wave50_slot_transform_fn selected = wave50_fnptr_return_slot_depth_callback(5, wave50_slot_first, wave50_slot_fallback);
    struct wave50_slot_cell got = selected(seed, 5);
    printf("%d %d %d %d\n", got.value[0], got.value[1], got.value[2], got.tag);
    return 0;
}
