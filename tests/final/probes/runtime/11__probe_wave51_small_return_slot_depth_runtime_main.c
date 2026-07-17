#include <stdio.h>

struct wave51_slot_pair {
    int x;
    int y;
};

typedef struct wave51_slot_pair (*wave51_slot_fn)(struct wave51_slot_pair item, int salt);

wave51_slot_fn wave51_small_return_slot_depth(int depth,
                                              wave51_slot_fn first,
                                              wave51_slot_fn second);

static struct wave51_slot_pair wave51_slot_first(struct wave51_slot_pair item, int salt) {
    struct wave51_slot_pair out;
    out.x = item.x + salt * 4;
    out.y = item.y - salt + 10;
    return out;
}

static struct wave51_slot_pair wave51_slot_second(struct wave51_slot_pair item, int salt) {
    struct wave51_slot_pair out;
    out.x = item.x * 3 - salt;
    out.y = item.y + salt * 5;
    return out;
}

int main(void) {
    struct wave51_slot_pair seed = {8, 12};
    wave51_slot_fn fn = wave51_small_return_slot_depth(7, wave51_slot_first, wave51_slot_second);
    struct wave51_slot_pair mid = fn(seed, 5);
    wave51_slot_fn again = wave51_small_return_slot_depth(mid.x - mid.y, wave51_slot_second, fn);
    struct wave51_slot_pair got = again(mid, 3);
    printf("%d %d\n", got.x, got.y);
    return 0;
}
