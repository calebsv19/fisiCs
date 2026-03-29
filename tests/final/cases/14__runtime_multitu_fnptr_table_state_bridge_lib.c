typedef int (*op_fn)(int, int);

typedef struct OpLane {
    op_fn fn;
    int bias;
} OpLane;

static int add_lane(int a, int b) { return a + b; }
static int xor_lane(int a, int b) { return (a ^ b) + 3; }
static int mix_lane(int a, int b) { return a * 2 - b; }
static int sub_lane(int a, int b) { return a - b; }

OpLane select_lane(int mode) {
    switch (mode & 3) {
        case 0: return (OpLane){add_lane, 5};
        case 1: return (OpLane){xor_lane, -3};
        case 2: return (OpLane){mix_lane, 7};
        default: return (OpLane){sub_lane, 11};
    }
}

int apply_lane(OpLane lane, int seed, int step) {
    return lane.fn(seed + lane.bias, step + 1);
}
