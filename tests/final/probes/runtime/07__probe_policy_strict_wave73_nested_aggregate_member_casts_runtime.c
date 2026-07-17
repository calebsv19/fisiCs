#include <stdio.h>

enum Scale {
    SCALE_LOW = -2,
    SCALE_HIGH = 7
};

struct Inner {
    unsigned char code;
    signed char delta;
    enum Scale scale;
};

struct Wrapper {
    struct Inner rows[2];
    unsigned int bias;
};

static int fold_member(struct Wrapper *wrapper, int pick) {
    struct Inner *member = &wrapper->rows[pick];
    unsigned int narrowed = (unsigned int)(unsigned char)(member->code + wrapper->bias);
    int promoted = (int)member->delta + (int)member->scale;
    unsigned int magnitude = (unsigned int)(promoted < 0 ? -promoted : promoted);
    unsigned int cross = (unsigned int)(unsigned short)(wrapper->rows[1 - pick].code * 2u);
    return (int)(narrowed + magnitude + cross);
}

int main(void) {
    struct Wrapper wrapper = {
        {{240u, -5, SCALE_HIGH}, {12u, 4, SCALE_LOW}},
        23u
    };

    printf("%d %d\n",
           fold_member(&wrapper, 0),
           fold_member(&wrapper, 1));
    return 0;
}
