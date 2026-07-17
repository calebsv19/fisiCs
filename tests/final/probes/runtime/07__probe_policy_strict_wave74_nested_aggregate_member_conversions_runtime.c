#include <stdio.h>

enum Bias {
    BIAS_DOWN = -3,
    BIAS_UP = 11
};

struct Inner {
    unsigned char raw;
    signed char delta;
};

union Cell {
    struct Inner inner;
    unsigned short packed;
};

struct Row {
    enum Bias bias;
    union Cell cells[2];
};

static int score_cell(struct Row *row, int index) {
    unsigned int raw = (unsigned int)row->cells[index].inner.raw;
    int delta = (int)row->cells[index].inner.delta;
    int bias = (int)row->bias;
    return (int)(unsigned char)(raw + bias) + (int)(signed char)(delta - bias);
}

int main(void) {
    struct Row rows[2] = {
        {BIAS_DOWN, {{{250u, -7}}, {{12u, 5}}}},
        {BIAS_UP, {{{33u, -2}}, {{199u, 4}}}}
    };

    struct Row *selected = 1 ? &rows[0] : &rows[1];
    int first = score_cell(selected, 0);
    int second = score_cell(&rows[1], 1);
    unsigned int packed = (unsigned int)(unsigned short)rows[0].cells[1].packed;
    int third = (int)(packed & 255u) + (int)((packed >> 8) & 31u);
    printf("%d %d %d\n", first, second, third);
    return 0;
}
