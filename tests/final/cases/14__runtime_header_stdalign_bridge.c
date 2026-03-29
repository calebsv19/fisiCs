#include <stddef.h>
#include <stdalign.h>
#include <stdio.h>

struct Wave85A {
    char c;
    int i;
};

struct Wave85B {
    char c;
    long double ld;
};

#ifndef __alignas_is_defined
#define __alignas_is_defined 0
#endif

#ifndef __alignof_is_defined
#define __alignof_is_defined 0
#endif

int main(void) {
    size_t a_align = _Alignof(struct Wave85A);
    size_t b_align = _Alignof(struct Wave85B);
    size_t i_align = _Alignof(int);
    size_t ld_align = _Alignof(long double);
    size_t off_b = offsetof(struct Wave85B, ld);

    printf(
        "%llu %llu %llu %llu %llu %d %d\n",
        (unsigned long long)a_align,
        (unsigned long long)b_align,
        (unsigned long long)i_align,
        (unsigned long long)ld_align,
        (unsigned long long)off_b,
        __alignas_is_defined,
        __alignof_is_defined
    );
    return 0;
}
