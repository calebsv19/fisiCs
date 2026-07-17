#include <stdio.h>

enum Kind {
    KIND_NEG = -3,
    KIND_SMALL = 5,
    KIND_BIG = 260
};

struct Cell {
    unsigned char code;
    signed short delta;
};

struct Box {
    enum Kind kind;
    struct Cell cells[2];
    unsigned long mask;
};

static int fold_box(struct Box *box, int index) {
    int converted_code = (int)(unsigned char)(box->cells[index].code + (int)box->kind);
    int converted_delta = (int)(signed char)box->cells[1 - index].delta;
    return converted_code + converted_delta + (int)(box->mask & 15ul);
}

int main(void) {
    struct Box boxes[2] = {
        {KIND_SMALL, {{250u, -7}, {12u, 4}}, 0x23ul},
        {KIND_NEG, {{2u, -9}, {200u, 8}}, 0x31ul}
    };

    struct Box *first = 1 ? &boxes[0] : &boxes[1];
    struct Box *second = 0 ? &boxes[0] : &boxes[1];
    unsigned int promoted = (unsigned char)(boxes[0].cells[0].code + boxes[1].kind);
    long mixed = (long)((int)boxes[1].kind + (unsigned char)boxes[1].cells[1].code);

    printf("%d %d %u %ld\n",
           fold_box(first, 0),
           fold_box(second, 1),
           promoted,
           mixed);
    return 0;
}
