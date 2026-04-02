struct Pair {
    int x;
    int y;
};

union Payload {
    struct Pair pair;
    int raw[2];
};

int main(void) {
    union Payload p;
    p.pair.x = 1;
    return p->pair.x;
}
