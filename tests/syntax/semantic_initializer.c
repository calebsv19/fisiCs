struct Pair {
    int x;
    int y;
};

int main(void) {
    int base = 3;
    struct Pair p = { .x = base, .y = base + 1 };
    int values[2] = { base, base + 2 };
    return p.y + values[1];
}
