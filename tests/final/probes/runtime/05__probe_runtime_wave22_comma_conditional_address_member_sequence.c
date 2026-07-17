extern int printf(const char*, ...);

struct Cell {
    int slots[3];
};

int main(void) {
    struct Cell a = {{2, 4, 6}};
    struct Cell b = {{3, 5, 7}};
    int choose = 1;
    int trace = 0;

    trace += 1;
    struct Cell *selected = choose ? &a : &b;
    trace += 2;
    int *slot = (int *)(void *)&selected->slots[1];
    *slot += (int)sizeof(((trace += 50), b.slots[0])) + (trace += 3, b.slots[2]);

    choose = 0;
    trace += 5;
    struct Cell *selected2 = choose ? &a : &b;
    trace += 7;
    int *slot2 = (int *)(void *)&selected2->slots[2];
    *slot2 += (trace += 11, *slot);

    printf("%d %d %d %d %d %d\n", trace, a.slots[0], a.slots[1], a.slots[2], b.slots[1], b.slots[2]);
    return 0;
}
