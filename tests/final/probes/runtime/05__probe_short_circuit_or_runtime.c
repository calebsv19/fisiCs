static int hits;

static int lhs(void) {
    hits += 1;
    return 1;
}

static int rhs(void) {
    hits += 10;
    return 0;
}

int main(void) {
    int value = lhs() || rhs();
    return (value == 1 && hits == 1) ? 0 : 1;
}
