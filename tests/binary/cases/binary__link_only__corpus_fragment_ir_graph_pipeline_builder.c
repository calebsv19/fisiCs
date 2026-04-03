typedef struct Node Node;
struct Node {
    int id;
    unsigned kind;
    Node *lhs;
    Node *rhs;
};

static Node *mk(Node *arena, unsigned cap, unsigned *used, unsigned kind, Node *lhs, Node *rhs) {
    if (*used >= cap) return (Node *)0;
    arena[*used].id = (int)(*used);
    arena[*used].kind = kind;
    arena[*used].lhs = lhs;
    arena[*used].rhs = rhs;
    (*used)++;
    return &arena[*used - 1];
}

Node *ir_build_sample(Node *arena, unsigned cap, unsigned *used) {
    Node *a = mk(arena, cap, used, 1u, 0, 0);
    Node *b = mk(arena, cap, used, 1u, 0, 0);
    Node *c = mk(arena, cap, used, 2u, a, b);
    return mk(arena, cap, used, 3u, c, a);
}
