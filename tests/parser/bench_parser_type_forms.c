typedef int Base;
typedef Base Vec4[4];
typedef Base (*UnaryFn)(Base);
typedef Base (*BinaryFn)(Base, Base);

typedef struct Node {
    Base value;
    struct Node* next;
} Node;

static Base inc(Base x) { return x + 1; }
static Base dec(Base x) { return x - 1; }
static Base add(Base x, Base y) { return x + y; }

static Base apply_unary(UnaryFn fn, Base x) {
    return fn(x);
}

static Base apply_binary(BinaryFn fn, Base x, Base y) {
    return fn(x, y);
}

static Base bench_type_forms(Base seed) {
    Vec4 local = { 1, 2, 3, 4 };
    UnaryFn unary_table[2] = { inc, dec };
    BinaryFn binary_table[1] = { add };
    Node nodes[2] = {
        { seed, 0 },
        { seed + 1, 0 }
    };
    Node* head = &nodes[0];
    Base (*(*pick_table)(int))[2];
    Base sum = 0;

    nodes[0].next = &nodes[1];
    head = head->next;

    sum += apply_unary(unary_table[0], seed);
    sum += apply_unary(unary_table[1], seed);
    sum += apply_binary(binary_table[0], local[0], local[1]);
    sum += ((Base)(local[2]));
    sum += ((Node){ seed + 2, head }).value;
    sum += ((Node){ seed + 3, head }).next->value;
    sum += sizeof(Vec4);
    sum += sizeof(UnaryFn);
    sum += pick_table == 0;
    return sum;
}

int main(void) {
    return bench_type_forms(7) == 0;
}
