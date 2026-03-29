#include <stddef.h>
#include <stdio.h>

struct HeaderBridge {
    char tag;
    int value;
    short tail;
};

int main(void) {
    int a[9] = {0};
    ptrdiff_t elem_delta = &a[8] - &a[1];
    size_t off_value = offsetof(struct HeaderBridge, value);
    size_t off_tail = offsetof(struct HeaderBridge, tail);
    size_t total = sizeof(struct HeaderBridge) + sizeof(size_t) + sizeof(ptrdiff_t);

    printf("%td %llu %llu %llu\n",
           elem_delta,
           (unsigned long long)off_value,
           (unsigned long long)off_tail,
           (unsigned long long)total);
    return 0;
}
