typedef struct Node Node;
struct Node {
    int id;
    unsigned kind;
    Node *lhs;
    Node *rhs;
};

Node *ir_build_sample(Node *arena, unsigned cap, unsigned *used);
unsigned ir_run_passes(Node *nodes, unsigned count);

int main(void) {
    Node arena[32];
    unsigned used = 0;
    Node *root = ir_build_sample(arena, 32u, &used);
    unsigned digest = ir_run_passes(arena, used);
    return (root ? 0 : 1) + (int)(digest & 1u);
}
