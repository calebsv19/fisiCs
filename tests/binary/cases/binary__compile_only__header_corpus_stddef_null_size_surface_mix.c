#include <stddef.h>

struct Wave22StddefNullSizeSurface {
    size_t count;
    ptrdiff_t delta;
    void *marker;
};

int wave22_stddef_null_size_surface(struct Wave22StddefNullSizeSurface *out, int *base, int *tail) {
    out->count = sizeof(*out);
    out->delta = tail - base;
    out->marker = NULL;
    return out->marker == NULL && out->delta >= 0;
}
