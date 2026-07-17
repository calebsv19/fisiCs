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
    int tail_salt) {
    struct wave55_compact first = transform(start, outer_salt);
    struct wave55_compact second = transform(first, tail_salt);

    second.y += outer_salt;
    second.z += tail_salt;
    return second;
}
