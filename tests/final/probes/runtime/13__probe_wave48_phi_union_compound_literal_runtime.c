#include <stdio.h>

typedef struct {
    int kind;
    union {
        int words[3];
        struct {
            int a;
            int b;
        } pair;
    } u;
    int stamp;
} Wave48Box;

static int score(Wave48Box box) {
    int total = box.kind * 17 + box.stamp;
    if (box.kind) {
        total += box.u.pair.a * 3 - box.u.pair.b * 5;
    } else {
        total += box.u.words[0] * 7 - box.u.words[1] * 11 + box.u.words[2] * 13;
    }
    return total;
}

int main(void) {
    Wave48Box box = {0, {{3, 5, 8}}, 2};
    int total = 0;
    int i;

    for (i = 0; i < 11; ++i) {
        Wave48Box candidate = ((score(box) + i) & 1)
            ? (Wave48Box){1, {.pair = {box.stamp + i, box.u.words[i % 3] - i}}, box.stamp + 4}
            : (Wave48Box){0, {{box.stamp + i, score(box) & 31, box.kind + i * 2}}, box.stamp - i};
        if ((score(candidate) ^ total) & 3) {
            box = candidate;
        } else {
            box.stamp += i + box.kind;
            if (box.kind) {
                box.u.pair.b -= i;
            } else {
                box.u.words[(i + 1) % 3] += box.stamp;
            }
        }
        total += score(box);
    }

    printf("%d %d %d %d\n", box.kind, box.stamp, score(box), total);
    return 0;
}
