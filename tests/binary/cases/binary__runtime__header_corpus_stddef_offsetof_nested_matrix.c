#include <stddef.h>
#include <stdio.h>

struct Wave22StddefNestedInner {
    short code;
    char flag;
    int value;
};

struct Wave22StddefNestedOuter {
    char lead;
    struct Wave22StddefNestedInner inner;
    long stamp;
};

int main(void) {
    size_t off_inner = offsetof(struct Wave22StddefNestedOuter, inner);
    size_t off_value = offsetof(struct Wave22StddefNestedOuter, inner.value);
    size_t off_stamp = offsetof(struct Wave22StddefNestedOuter, stamp);
    size_t total = off_inner + off_value + off_stamp + sizeof(struct Wave22StddefNestedOuter);

    printf("stddef-offsets inner=%lu value=%lu stamp=%lu total=%lu\n",
           (unsigned long)off_inner,
           (unsigned long)off_value,
           (unsigned long)off_stamp,
           (unsigned long)total);
    return total == (size_t)52 ? 0 : 1;
}
