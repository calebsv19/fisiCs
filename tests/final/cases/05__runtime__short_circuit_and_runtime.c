static int hits;

static int lhs(void) {
    hits += 1;
    return 0;
}

static int rhs(void) {
    hits += 10;
    return 1;
}

int main(void) {
    int value = lhs() && rhs();
    return (value == 0 && hits == 1) ? 0 : 1;
}
