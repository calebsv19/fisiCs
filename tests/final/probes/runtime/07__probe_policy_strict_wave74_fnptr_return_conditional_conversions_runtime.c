#include <stdio.h>

typedef int (*op_fn)(int, unsigned char);

enum Mode {
    MODE_ADD = 0,
    MODE_SUB = 1,
    MODE_MIX = 2
};

static int add_op(int seed, unsigned char value) {
    return (int)(unsigned char)(seed + (int)value);
}

static int sub_op(int seed, unsigned char value) {
    return (int)(signed char)(seed - (int)value);
}

static int mix_op(int seed, unsigned char value) {
    return (int)(unsigned char)((seed ^ (int)value) + 3);
}

static op_fn choose(enum Mode mode, int flip) {
    op_fn add = add_op;
    op_fn sub = sub_op;
    op_fn mix = mix_op;
    return mode == MODE_ADD ? add : (flip ? mix : sub);
}

static int apply(op_fn (*chooser)(enum Mode, int), enum Mode mode, int flip, int seed, unsigned char value) {
    op_fn selected = chooser(mode, flip);
    op_fn fallback = 0 ? sub_op : selected;
    return fallback(seed, value);
}

int main(void) {
    op_fn (*chooser)(enum Mode, int) = choose;
    int first = apply(chooser, MODE_ADD, 0, 250, 12u);
    int second = apply(chooser, MODE_SUB, 0, -120, 25u);
    int third = apply(chooser, MODE_MIX, 1, 33, 44u);
    printf("%d %d %d\n", first, second, third);
    return 0;
}
