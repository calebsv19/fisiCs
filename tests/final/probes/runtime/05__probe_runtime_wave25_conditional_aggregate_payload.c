extern int printf(const char*, ...);

struct Cell {
    int head;
    int slots[2];
};

static struct Cell make_cell(int *trace, int delta) {
    *trace += delta;
    struct Cell cell = {*trace, {*trace + 1, *trace + 2}};
    return cell;
}

int main(void) {
    int trace = 3;
    int choose = 1;

    struct Cell picked = choose ? make_cell(&trace, 4) : make_cell(&trace, 40);
    struct Cell copy = (choose && picked.slots[1] > 0) ? picked : make_cell(&trace, 80);
    int total = picked.head + copy.slots[1];

    printf("%d %d %d %d %d\n", trace, picked.head, picked.slots[0], copy.slots[1], total);
    return 0;
}
