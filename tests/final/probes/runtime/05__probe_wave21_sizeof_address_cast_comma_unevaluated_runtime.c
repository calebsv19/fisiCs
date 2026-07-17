extern int printf(const char*, ...);

struct Box {
    int value;
    int alt;
};

static int mark(int *slot, int amount) {
    *slot += amount;
    return *slot;
}

int main(void) {
    struct Box boxes[2] = {
        {3, 5},
        {7, 11},
    };
    struct Box *p = boxes;
    int ticks = 0;

    int width = (int)sizeof((int)(mark(&ticks, 100), (p + 1)->alt));
    int selected = ticks == 0
        ? ((int)sizeof(*(p + (mark(&ticks, 7), 1))), mark(&ticks, 2))
        : 99;

    printf("%d %d %d %d\n", ticks, width, selected, boxes[1].alt);
    return 0;
}
