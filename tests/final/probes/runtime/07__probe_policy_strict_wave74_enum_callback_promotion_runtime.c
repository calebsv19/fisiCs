#include <stdio.h>

enum Kind {
    KIND_NEG = -6,
    KIND_LOW = 5,
    KIND_HIGH = 140
};

typedef int (*fold_fn)(enum Kind, unsigned char, signed char);

static int fold_signed(enum Kind kind, unsigned char payload, signed char adjust) {
    int promoted = kind + adjust;
    unsigned int mixed = (unsigned int)(unsigned char)(payload + kind);
    return promoted + (int)(mixed & 63u);
}

static int fold_wide(enum Kind kind, unsigned char payload, signed char adjust) {
    unsigned long wide = (unsigned long)((unsigned int)payload + (unsigned int)kind);
    int branch = kind < (unsigned int)3 ? 19 : 7;
    return (int)(wide & 127ul) + (int)adjust + branch;
}

static int dispatch(fold_fn fn, enum Kind kind, unsigned char payload, signed char adjust) {
    return fn(kind, payload, adjust);
}

int main(void) {
    fold_fn table[2] = {fold_signed, fold_wide};
    enum Kind kinds[3] = {KIND_NEG, KIND_LOW, KIND_HIGH};
    int first = dispatch(table[0], kinds[0], 250u, -4);
    int second = dispatch(table[1], kinds[2], 17u, 3);
    int third = (1 ? table[0] : table[1])(kinds[1], 12u, -2);
    printf("%d %d %d\n", first, second, third);
    return 0;
}
