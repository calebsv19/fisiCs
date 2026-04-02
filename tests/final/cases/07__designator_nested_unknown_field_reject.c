struct Inner {
    int x;
    int y;
};

struct Outer {
    struct Inner in;
};

struct Outer g = {.in = {.z = 9}};

int main(void) {
    return g.in.x;
}

