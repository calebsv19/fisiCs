#include <stddef.h>
#include <stdint.h>
#include <stdio.h>

struct Wave85Node {
    char tag;
    long long value;
};

int main(void) {
    struct Wave85Node* p = NULL;
    struct Wave85Node a[6];

    ptrdiff_t d = &a[5] - &a[2];
    size_t sz_node = sizeof(*a);
    size_t sz_ptr = sizeof(p);
    size_t sz_pd = sizeof(ptrdiff_t);
    uintptr_t z = (uintptr_t)NULL;
    int null_ok = (p == NULL) && (z == 0u);

    printf(
        "%d %td %llu %llu %llu\n",
        null_ok,
        d,
        (unsigned long long)sz_node,
        (unsigned long long)sz_ptr,
        (unsigned long long)sz_pd
    );
    return 0;
}
