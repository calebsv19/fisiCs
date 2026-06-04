#include <stddef.h>

struct Wave22StddefInner {
    char tag;
    int value;
};

struct Wave22StddefOuter {
    char lead;
    struct Wave22StddefInner inner;
    long stamp;
};

enum {
    wave22_offset_inner_ok = 1 / (offsetof(struct Wave22StddefOuter, inner) > 0),
    wave22_offset_nested_ok = 1 / (offsetof(struct Wave22StddefOuter, inner.value) > offsetof(struct Wave22StddefOuter, inner)),
    wave22_offset_stamp_ok = 1 / (offsetof(struct Wave22StddefOuter, stamp) > offsetof(struct Wave22StddefOuter, inner.value))
};

size_t wave22_stddef_offset_surface(void) {
    return offsetof(struct Wave22StddefOuter, inner.value) + offsetof(struct Wave22StddefOuter, stamp);
}
