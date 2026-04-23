#include <stdarg.h>
#include <stdio.h>

typedef struct {
    unsigned long long fold;
    unsigned count;
} Payload;

typedef Payload (*Collector)(unsigned seed, const char* spec, ...);

static Payload collect_mix(unsigned seed, const char* spec, ...) {
    va_list ap;
    Payload p;
    const char* s = spec;

    p.fold = (unsigned long long)seed * 97ull + 13ull;
    p.count = 0u;

    va_start(ap, spec);
    while (*s != '\0') {
        if (*s == 'i') {
            int v = va_arg(ap, int);
            p.fold = p.fold * 131ull + (unsigned long long)(unsigned)v;
            p.count += 1u;
        } else if (*s == 'u') {
            unsigned v = va_arg(ap, unsigned);
            p.fold = p.fold * 137ull + (unsigned long long)v;
            p.count += 1u;
        } else if (*s == 'l') {
            unsigned long long v = va_arg(ap, unsigned long long);
            p.fold = p.fold * 139ull + v;
            p.count += 1u;
        }
        ++s;
    }
    va_end(ap);
    return p;
}

static Payload collect_alt(unsigned seed, const char* spec, ...) {
    va_list ap;
    Payload p;
    const char* s = spec;

    p.fold = (unsigned long long)seed * 89ull + 7ull;
    p.count = 0u;

    va_start(ap, spec);
    while (*s != '\0') {
        if (*s == 'i') {
            int v = va_arg(ap, int);
            p.fold = p.fold * 149ull + (unsigned long long)(unsigned)(v ^ 3);
            p.count += 1u;
        } else if (*s == 'u') {
            unsigned v = va_arg(ap, unsigned);
            p.fold = p.fold * 151ull + (unsigned long long)(v + 5u);
            p.count += 1u;
        } else if (*s == 'l') {
            unsigned long long v = va_arg(ap, unsigned long long);
            p.fold = p.fold * 157ull + (v ^ 0x33ull);
            p.count += 1u;
        }
        ++s;
    }
    va_end(ap);
    return p;
}

int main(void) {
    Collector table[2];
    Payload a;
    Payload b;
    unsigned digest;

    table[0] = collect_mix;
    table[1] = collect_alt;

    a = table[0](19u, "iul", -7, 23u, 10000000007ull);
    b = table[1](31u, "lui", 4000000005ull, 17u, -3);

    digest = (unsigned)(a.fold ^ (a.fold >> 32));
    digest = digest * 167u + a.count;
    digest = digest * 167u + (unsigned)(b.fold ^ (b.fold >> 32));
    digest = digest * 167u + b.count;

    printf("%u %u %u %u\n",
           (unsigned)(a.fold & 0xffffffffu),
           (unsigned)(b.fold & 0xffffffffu),
           a.count + b.count,
           digest);
    return 0;
}
