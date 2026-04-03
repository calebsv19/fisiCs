#include <stddef.h>

typedef enum {
    NODE_INT = 1,
    NODE_ADD = 2,
    NODE_SUB = 3,
    NODE_MUL = 4
} NodeKind;

typedef struct Node Node;
struct Node {
    NodeKind kind;
    int value;
    Node *lhs;
    Node *rhs;
};

static int eval(const Node *n) {
    switch (n->kind) {
        case NODE_INT: return n->value;
        case NODE_ADD: return eval(n->lhs) + eval(n->rhs);
        case NODE_SUB: return eval(n->lhs) - eval(n->rhs);
        case NODE_MUL: return eval(n->lhs) * eval(n->rhs);
    }
    return 0;
}

int eval_forest(const Node *nodes, size_t count) {
    size_t i;
    int acc = 0;
    for (i = 0; i < count; ++i) {
        acc += eval(&nodes[i]);
    }
    return acc;
}
