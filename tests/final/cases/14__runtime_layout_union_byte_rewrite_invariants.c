#include <stdio.h>

int main(void) {
    unsigned int x = 0x12345678u;
    unsigned int old = x;
    unsigned char* p = (unsigned char*)&x;

    p[0] ^= 0x5Au;
    p[sizeof(unsigned int) - 1] ^= 0xA5u;

    int changed = (x != old);

    unsigned int y = 0u;
    unsigned char* q = (unsigned char*)&y;
    for (unsigned i = 0; i < sizeof(unsigned int); ++i) {
        q[i] = p[i];
    }

    int same = 1;
    for (unsigned i = 0; i < sizeof(unsigned int); ++i) {
        if (q[i] != p[i]) {
            same = 0;
        }
    }

    for (unsigned i = 0; i < sizeof(unsigned int); ++i) {
        q[i] ^= (unsigned char)(i * 17u + 3u);
    }
    for (unsigned i = 0; i < sizeof(unsigned int); ++i) {
        q[i] ^= (unsigned char)(i * 17u + 3u);
    }

    int restored = 1;
    for (unsigned i = 0; i < sizeof(unsigned int); ++i) {
        if (q[i] != p[i]) {
            restored = 0;
        }
    }

    printf("%d %d %d\n", changed, same, restored);
    return 0;
}
