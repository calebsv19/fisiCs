extern int printf(const char*, ...);

static int hits;

static int lhs(void) {
    hits += 1;
    return 1;
}

static int rhs(void) {
    hits += 100;
    return 0;
}

int main(void) {
    int value = lhs() || rhs();
    printf("%d %d\n", hits, value);
    return 0;
}
