#include <stddef.h>
#include <stdio.h>

union HeaderPayload {
    int i32;
    short s16[3];
};

struct HeaderTail {
    char c;
    long long wide;
};

struct HeaderNest {
    char tag;
    union HeaderPayload payload;
    struct HeaderTail tail;
};

int main(void) {
    size_t o_payload = offsetof(struct HeaderNest, payload);
    size_t o_tail = offsetof(struct HeaderNest, tail);
    size_t o_tail_wide = offsetof(struct HeaderNest, tail.wide);
    size_t total = sizeof(struct HeaderNest) + sizeof(size_t);

    printf("%llu %llu %llu %llu\n",
           (unsigned long long)o_payload,
           (unsigned long long)o_tail,
           (unsigned long long)o_tail_wide,
           (unsigned long long)total);
    return 0;
}
