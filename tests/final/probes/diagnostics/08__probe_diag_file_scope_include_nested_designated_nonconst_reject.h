struct Inner {
    int x;
};

struct Outer {
    struct Inner inner;
    int arr[2];
};

struct Outer state = {
    .inner = {.x = seed()},
    .arr = {[1] = 9},
};
