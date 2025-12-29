// Regression: flexible array element lvalues should codegen without crashing.
// We set and read f->data[N] through a flex member.

struct Flex {
    int n;
    int data[];
};

static int test(void) {
    char buf[sizeof(struct Flex) + 3 * sizeof(int)];
    struct Flex* f = (struct Flex*)buf;
    f->n = 3;
    f->data[0] = 10;
    f->data[1] = 20;
    f->data[2] = 30;
    return f->data[2] - f->data[0];
}

int main(void) {
    return test();
}
