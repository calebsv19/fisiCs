// Regression: real-project Stage A blocker family (workspace_sandbox)
// Pointer-return helpers must accept null pointer constants (`return 0;`).

#include <stdio.h>

typedef struct WsNode {
    int value;
} WsNode;

static WsNode *lookup_node(int key) {
    static WsNode node = {7};
    if (key == 0) {
        return 0;
    }
    return &node;
}

static const char *lookup_label(int key) {
    if (key == 0) {
        return 0;
    }
    return "x";
}

int main(void) {
    WsNode *a = lookup_node(0);
    WsNode *b = lookup_node(1);
    const char *s = lookup_label(0);
    const char *t = lookup_label(1);
    printf("%d %d %d %d\n",
           a == 0,
           (b && b->value == 7),
           s == 0,
           (t && t[0] == 'x'));
    return 0;
}
