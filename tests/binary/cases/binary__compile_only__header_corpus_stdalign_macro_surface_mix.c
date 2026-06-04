#include <stdalign.h>
#include <stddef.h>

struct Wave15MacroSurface {
    char lead;
    alignas(16) unsigned char payload[16];
};

struct Wave15MaxAlignSurface {
    alignas(max_align_t) unsigned char slot[sizeof(max_align_t)];
    long double value;
};

_Static_assert(__alignas_is_defined == 1, "stdalign alignas macro must be defined");
_Static_assert(__alignof_is_defined == 1, "stdalign alignof macro must be defined");
_Static_assert(alignof(struct Wave15MacroSurface) >= 16, "alignas(16) must raise struct alignment");
_Static_assert((offsetof(struct Wave15MacroSurface, payload) % 16) == 0, "payload must stay 16-byte aligned");
_Static_assert(alignof(struct Wave15MaxAlignSurface) >= alignof(max_align_t), "max_align_t bridge must preserve alignment");

int header_corpus_wave15_macro_surface(void) {
    return (int)alignof(struct Wave15MaxAlignSurface);
}
