struct Leaf {
    int a;
    int b;
};

struct Node {
    struct Leaf leaves[2];
    int tail;
};

static struct Node g_nodes[2] = {
    [0] = {
        .leaves = {
            [0] = { .a = 1, .b = 2 },
            [1] = { .a = 3, .b = 4 }
        },
        .tail = 5
    },
    [1] = {
        .leaves = {
            [0] = { .a = 6, .b = 7 },
            [1] = { .a = 8, .b = }
        },
        .tail = 9
    }
};

int main(void) {
    return g_nodes[0].tail;
}
