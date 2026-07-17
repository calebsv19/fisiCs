#include <stddef.h>
#include <stdio.h>

struct S {
    unsigned int value;
    unsigned int salt;
};

int main(void) {
    struct S items[5] = {
        {3u, 5u},
        {7u, 11u},
        {13u, 17u},
        {19u, 23u},
        {29u, 31u},
    };
    struct S *base = items;
    struct S *cursor = &items[1];
    struct S *limit = &items[4];
    unsigned int checksum;
    ptrdiff_t forward;
    ptrdiff_t reverse;

    {
        struct S {
            long inner_value;
            long extra;
        } inner = {101L, 103L};
        unsigned int inner_guard = (unsigned int)(inner.inner_value + inner.extra);

        checksum = (cursor++)->value * 3u;
        checksum += (++cursor)->value * 5u;
        checksum += (cursor--)->salt * 7u;
        checksum += (--cursor)->salt * 11u;
        checksum += inner_guard * 13u;
        forward = limit - cursor;
        reverse = cursor - limit;
    }

    printf("%u %td %td %td %td\n", checksum, forward, reverse,
           cursor - base, limit - base);
    return 0;
}
