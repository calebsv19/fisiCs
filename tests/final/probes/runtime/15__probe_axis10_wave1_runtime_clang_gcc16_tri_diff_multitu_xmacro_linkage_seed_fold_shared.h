#ifndef AXIS10_XMACRO_LINKAGE_SHARED_H
#define AXIS10_XMACRO_LINKAGE_SHARED_H

#define AXIS10_OPS(X) \
    X(advance, 3u) \
    X(merge, 5u) \
    X(rotate, 7u) \
    X(finalize, 11u)

#define AXIS10_ENUM(name, weight) AXIS10_OP_##name,
enum axis10_op {
    AXIS10_OPS(AXIS10_ENUM)
    AXIS10_OP_COUNT
};
#undef AXIS10_ENUM

#define AXIS10_TAG(name) axis10_##name
#define AXIS10_SEED(base, delta) ((base) + (delta))

struct axis10_state {
    unsigned lanes[AXIS10_OP_COUNT];
    unsigned digest;
};

struct axis10_state axis10_apply(struct axis10_state input, enum axis10_op op,
                                 unsigned seed);

#endif
