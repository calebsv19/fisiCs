#include <stdio.h>

typedef union {
    struct {
        int pair[2];
        int scale;
    } vec;
    struct {
        int left;
        int right;
        int extra;
    } alt;
} Wave42Union;

typedef struct {
    int kind;
    Wave42Union u;
    int stamp;
} Wave42Box;

static int box_score(Wave42Box b) {
    if (b.kind == 1) {
        return b.u.vec.pair[0] * b.u.vec.scale + b.u.vec.pair[1] + b.stamp;
    }
    return b.u.alt.left - b.u.alt.right + b.u.alt.extra * 2 + b.stamp;
}

int main(void) {
    Wave42Box box = (Wave42Box){1, {.vec = {{2, 5}, 3}}, 11};
    int total = 0;
    int i;

    for (i = 0; i < 7; ++i) {
        Wave42Box next = box;
        switch ((i + box.kind + box.stamp) % 4) {
            case 0:
                next = (Wave42Box){1, {.vec = {{i + 3, i + 6}, i + 2}}, i + 30};
                break;
            case 1:
                next = (Wave42Box){2, {.alt = {i + 10, i + 4, i + 1}}, i + 40};
                break;
            case 2:
                box.stamp += i + 1;
                total += box_score(box);
                continue;
            default:
                next = (box_score(box) & 1)
                    ? (Wave42Box){2, {.alt = {box.kind + i, box.stamp - i, i + 5}}, i + 50}
                    : (Wave42Box){1, {.vec = {{box.kind + i, box.stamp + i}, 2}}, i + 60};
                break;
        }

        if (box_score(next) >= box_score(box)) {
            box = next;
        } else {
            box.stamp += i + box.kind;
        }
        total += box_score(box);
    }

    printf("%d %d %d %d\n", box.kind, box.stamp, box_score(box), total);
    return 0;
}
