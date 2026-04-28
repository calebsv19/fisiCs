struct Outer {
    int tag;
    union {
        struct {
            int left;
            int right;
        } pair;
        struct {
            int block;
        };
    };
};

static int use_outer(struct Outer* outer) {
    outer->block = 7;
    outer->pair.right = outer->block + 1;
    return outer->pair.right;
}

int main(void) {
    struct Outer outer = {0};
    return use_outer(&outer) == 8 ? 0 : 1;
}
