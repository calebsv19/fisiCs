struct Pair {
    int x;
    int y;
};

int main(void) {
    struct Pair p = { .x = 10, .y = 20 };
    return p.x + p.y;
}
