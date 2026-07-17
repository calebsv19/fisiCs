#include <stdio.h>

struct wave55_compact {
    int x;
    int y;
    int z;
};

typedef struct wave55_compact (*wave55_compact_transform_fn)(struct wave55_compact value, int salt);

struct wave55_compact wave55_typedef_callback_compact_aggregate(
    struct wave55_compact start,
    wave55_compact_transform_fn transform,
    int outer_salt,
    int tail_salt);

static struct wave55_compact wave55_transform(struct wave55_compact value, int salt) {
    struct wave55_compact out;

    out.x = value.y + salt;
    out.y = value.z * 2 - salt;
    out.z = value.x + value.y + salt;
    return out;
}

int main(void) {
    struct wave55_compact start = {3, 5, 7};
    struct wave55_compact got = wave55_typedef_callback_compact_aggregate(
        start, wave55_transform, 4, 9);

    printf("%d %d %d\n", got.x, got.y, got.z);
    return 0;
}
