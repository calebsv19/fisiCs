typedef int I;

struct Container {
    int x;
};

typedef struct Container Container;

union Data {
    int value;
};

int main(void) {
    union Data d;
    d.value = 42;
    return d.value;
}
