#include <stdalign.h>
#include <stddef.h>

struct Wave15NestedStorage {
    char prefix;
    _Alignas(8) short lane[2];
    int tail;
};

alignas(32) static unsigned char g_wave15_align_buffer[32];

struct Wave15OuterStorage {
    char c;
    struct Wave15NestedStorage nested;
};

_Static_assert(alignof(struct Wave15NestedStorage) >= 8, "nested storage must retain 8-byte alignment");
_Static_assert((offsetof(struct Wave15NestedStorage, lane) % 8) == 0, "lane field must start on 8-byte boundary");
_Static_assert(alignof(struct Wave15OuterStorage) >= 8, "outer aggregate must preserve nested alignment");
_Static_assert(sizeof(g_wave15_align_buffer) == 32, "aligned storage size must remain stable");

int header_corpus_wave15_storage_surface(void) {
    alignas(16) int local = 4;
    return local + (int)alignof(struct Wave15NestedStorage) + (int)(sizeof(g_wave15_align_buffer) / 32u);
}
