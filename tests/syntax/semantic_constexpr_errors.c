int g = 1;
int static_bad = g; // non-constant initializer at file scope

enum Trouble {
    GOOD = 0,
    BAD = g,
    NEXT
};

int demo(int v) {
    switch (v) {
        case v:
            return 1;
        default:
            return 0;
    }
}
