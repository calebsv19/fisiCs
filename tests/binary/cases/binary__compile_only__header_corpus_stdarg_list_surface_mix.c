#include <stdarg.h>
#include <stddef.h>

struct Wave17StdargListSurface {
    const char *spec;
    int count;
    long total;
};

static long wave17_stdarg_fold_ints(int count, va_list ap) {
    long total = 0;
    int i;
    for (i = 0; i < count; ++i) {
        total += (long)va_arg(ap, int);
    }
    return total;
}

long wave17_stdarg_list_surface(struct Wave17StdargListSurface *out, const char *spec, int count, ...) {
    va_list ap;
    long total;

    va_start(ap, count);
    total = wave17_stdarg_fold_ints(count, ap);
    va_end(ap);

    if (out != NULL) {
        out->spec = spec;
        out->count = count;
        out->total = total;
    }
    return total;
}
