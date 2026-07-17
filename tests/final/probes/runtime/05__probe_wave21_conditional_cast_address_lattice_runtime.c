extern int printf(const char*, ...);

struct Cell {
    int x;
    int y;
};

static int mark(int *slot, int amount) {
    *slot += amount;
    return *slot;
}

int main(void) {
    struct Cell cells[2] = {
        {2, 4},
        {6, 8},
    };
    int trace = 0;
    int flip = 1;

    int *chosen = flip
        ? (int *)(void *)&cells[1].y
        : (int *)(void *)&cells[0].x;
    *chosen += flip ? (mark(&trace, 3), 5) : (mark(&trace, 30), 9);

    int *other = (flip = 0, flip)
        ? (int *)(void *)&cells[0].y
        : (trace += 7, (int *)(void *)&cells[1].x);
    *other = *chosen + (trace += 11, cells[0].y);

    printf("%d %d %d %d %d\n", trace, cells[0].x, cells[0].y, cells[1].x, cells[1].y);
    return 0;
}
