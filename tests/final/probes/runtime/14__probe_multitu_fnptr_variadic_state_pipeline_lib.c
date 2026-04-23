#include <stdarg.h>

typedef unsigned (*MvsFn)(unsigned seed, const char* spec, ...);

static unsigned g_state = 0u;

static unsigned mvs_fold_core(unsigned seed, const char* spec, va_list ap, unsigned salt) {
    const char* s = spec;
    unsigned acc = seed * 97u + salt;

    while (*s != '\0') {
        if (*s == 'i') {
            int v = va_arg(ap, int);
            acc = acc * 131u + (unsigned)(v + (int)salt);
        } else if (*s == 'u') {
            unsigned v = va_arg(ap, unsigned);
            acc = acc * 137u + (v ^ salt);
        } else if (*s == 'l') {
            unsigned long long v = va_arg(ap, unsigned long long);
            acc = acc * 149u + (unsigned)(v ^ (unsigned long long)(salt * 3u));
        }
        ++s;
    }
    return acc;
}

static unsigned mvs_fold_a(unsigned seed, const char* spec, ...) {
    va_list ap;
    unsigned out;

    va_start(ap, spec);
    out = mvs_fold_core(seed, spec, ap, 19u);
    va_end(ap);

    g_state = g_state * 173u + out + 5u;
    return out;
}

static unsigned mvs_fold_b(unsigned seed, const char* spec, ...) {
    va_list ap;
    unsigned out;

    va_start(ap, spec);
    out = mvs_fold_core(seed, spec, ap, 43u);
    va_end(ap);

    g_state = g_state * 181u + out + 11u;
    return out;
}

void mvs_reset(void) {
    g_state = 23u;
}

MvsFn mvs_pick(unsigned selector) {
    if ((selector & 1u) == 0u) {
        return mvs_fold_a;
    }
    return mvs_fold_b;
}

unsigned mvs_digest(void) {
    return g_state;
}
