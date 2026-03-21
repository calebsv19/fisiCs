#include <stddef.h>
#include <stdio.h>

typedef struct {
    char tag;
    int value;
    short code;
} Node;

int main(void) {
    Node nodes[3] = {
        {1, 100, 7},
        {2, 200, 8},
        {3, 300, 9},
    };

    char* base = (char*)&nodes[0];
    Node* mid = (Node*)(base + sizeof(Node));

    int ok1 = (((char*)&nodes[2] - base) == (ptrdiff_t)(2 * sizeof(Node)));
    int ok2 = (mid->value == 200);
    int ok3 = (((char*)&nodes[1] - base) == (ptrdiff_t)sizeof(Node));
    int ok4 = (offsetof(Node, value) > offsetof(Node, tag));

    printf("%d %d %d %d\n", ok1, ok2, ok3, ok4);
    return 0;
}
