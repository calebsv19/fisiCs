#include <stdlib.h>

typedef void* (*AllocFn)(size_t);
typedef void* (*ReallocFn)(void*, size_t);
typedef void (*FreeFn)(void*);

typedef struct Hooks {
    AllocFn allocate;
    ReallocFn reallocate;
    FreeFn deallocate;
} Hooks;

typedef struct ProbeItem {
    int a;
    int b;
} ProbeItem;

void __fisics_memcheck_report(void);

static Hooks hooks = { malloc, realloc, free };

int main(void) {
    char* p = (char*)hooks.allocate(8);
    if (!p) return 2;
    p[0] = 'x';
    p[1] = '\0';

    p = (char*)hooks.reallocate(p, 32);
    if (!p) return 3;

    hooks.deallocate(p);

    size_t capacity = 4u;
    ProbeItem* items = (ProbeItem*)realloc(NULL, capacity * sizeof(*items));
    if (!items) return 4;
    if (capacity * sizeof(*items) != 32u) return 5;
    free(items);

    __fisics_memcheck_report();
    return 0;
}
