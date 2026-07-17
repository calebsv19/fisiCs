#include <stdio.h>

typedef int (*transform_fn)(int);

struct Dispatch {
    transform_fn transforms[2];
    int samples[4];
};

static int add_five(int value) {
    return value + 5;
}

static int triple(int value) {
    return value * 3;
}

static int apply(transform_fn transform, const int values[], int index) {
    return transform(values[index]);
}

int main(void) {
    struct Dispatch dispatch = {{add_five, triple}, {4, 9, 16, 25}};
    transform_fn selected = dispatch.transforms[1 ? 1 : 0];
    const int *decayed = 0 ? dispatch.samples + 1 : dispatch.samples;
    int first = apply(selected, decayed, 1);
    int second = apply(dispatch.transforms[0], &dispatch.samples[0], 3);

    printf("%d %d %d\n", first, second, decayed == dispatch.samples);
    return 0;
}
