typedef struct Node Node;
struct Node {
    int id;
    unsigned kind;
    Node *lhs;
    Node *rhs;
};

static unsigned hash_mix(unsigned h, unsigned v) {
    h ^= v + 0x9e3779b9u + (h << 6u) + (h >> 2u);
    return h;
}

unsigned ir_run_passes(Node *nodes, unsigned count) {
    unsigned i;
    unsigned h = 2166136261u;
    for (i = 0; i < count; ++i) {
        unsigned op = nodes[i].kind;
        if (op == 2u && nodes[i].lhs && nodes[i].rhs) {
            nodes[i].kind = 4u;
        }
        h = hash_mix(h, nodes[i].kind ^ (unsigned)nodes[i].id);
    }
    return h;
}
