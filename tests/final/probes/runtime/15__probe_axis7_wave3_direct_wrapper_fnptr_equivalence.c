#include <stdio.h>

typedef unsigned int (*Transform)(unsigned int, unsigned int);

static unsigned int blend(unsigned int value, unsigned int salt) {
    return (value * 17u) ^ (salt * 29u) ^ (value >> 3u);
}

static unsigned int direct_apply(Transform transform, unsigned int value,
                                 unsigned int salt) {
    return transform(value, salt);
}

static unsigned int wrapper_apply(Transform transform, unsigned int value,
                                  unsigned int salt) {
    Transform local = transform;
    return local(value, salt);
}

int main(void) {
    static const unsigned int values[] = {3u, 19u, 77u, 251u, 1024u};
    unsigned int direct = 0u;
    unsigned int wrapped = 0u;
    unsigned int i;

    for (i = 0u; i < 5u; ++i) {
        unsigned int salt = i * 13u + 7u;
        direct += direct_apply(blend, values[i], salt) * (i + 1u);
        wrapped += wrapper_apply(blend, values[i], salt) * (i + 1u);
    }
    printf("%u %u %u\n", direct, wrapped, direct == wrapped);
    return 0;
}
