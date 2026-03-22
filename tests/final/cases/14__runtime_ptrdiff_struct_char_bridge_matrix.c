#include <stddef.h>
#include <stdio.h>

typedef struct Node {
    int a;
    short b;
    char c[5];
} Node;

int main(void) {
    Node nodes[9];
    for (int i = 0; i < 9; ++i) {
        nodes[i].a = i * 17 + 5;
        nodes[i].b = (short)(i * 3 - 4);
        for (int j = 0; j < 5; ++j) {
            nodes[i].c[j] = (char)('A' + i + j);
        }
    }

    Node *n0 = &nodes[1];
    Node *n1 = &nodes[7];
    ptrdiff_t typed = n1 - n0;

    char *b0 = (char*)n0;
    char *b1 = (char*)n1;
    ptrdiff_t bytes = b1 - b0;
    ptrdiff_t roundtrip = bytes / (ptrdiff_t)sizeof(Node);

    char *inner0 = &n0->c[1];
    char *inner1 = &n1->c[3];
    ptrdiff_t inner = inner1 - inner0;

    printf("%lld %lld %lld %lld %d %d\n",
           (long long)typed,
           (long long)roundtrip,
           (long long)bytes,
           (long long)inner,
           n0->a,
           (int)n1->b);
    return 0;
}

