struct Inner {
    int x;
};

struct Outer {
    struct Inner inner;
    int arr[2];
};

int seed(void) {
    return 6;
}

struct Outer state = {
    .inner = {.x = seed()},
    .arr = {[1] = 9},
};

int main(void) {
    return 0;
}
