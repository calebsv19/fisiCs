struct Node {
    int value;
};

struct Node {
    int value;
    int extra;
};

union Payload {
    int i;
};

union Payload {
    float f;
};

enum Shade {
    SHADE_RED,
    SHADE_BLUE
};

enum Shade {
    SHADE_RED = 1,
    SHADE_BLUE,
    SHADE_GREEN
};

enum Color {
    COLOR_RED,
    COLOR_GREEN
};

int color_sum(enum Color c) {
    return c + COLOR_RED;
}
