#include <stdio.h>

typedef union {
    struct {
        int x;
        int y;
    } point;
    int raw[2];
} Wave42Cell;

typedef struct {
    int mode;
    Wave42Cell cells[3];
    int guard;
} Wave42Row;

static Wave42Row make_points(int seed) {
    Wave42Row r;
    int i;
    r.mode = 1;
    r.guard = seed + 100;
    for (i = 0; i < 3; ++i) {
        r.cells[i].point.x = seed + i * 2;
        r.cells[i].point.y = seed + i * 3 + 1;
    }
    return r;
}

static Wave42Row make_raw(int seed) {
    Wave42Row r;
    int i;
    r.mode = 2;
    r.guard = seed + 200;
    for (i = 0; i < 3; ++i) {
        r.cells[i].raw[0] = seed * 2 + i;
        r.cells[i].raw[1] = seed * 3 - i;
    }
    return r;
}

static int row_score(Wave42Row r) {
    int total = r.guard + r.mode;
    int i;
    for (i = 0; i < 3; ++i) {
        if (r.mode == 1) {
            total += r.cells[i].point.x * (i + 2) - r.cells[i].point.y;
        } else {
            total += r.cells[i].raw[0] - r.cells[i].raw[1] * (i + 1);
        }
    }
    return total;
}

int main(void) {
    Wave42Row current = make_points(4);
    int total = 0;
    int i;

    for (i = 0; i < 8; ++i) {
        Wave42Row candidate;
        switch ((row_score(current) + i) & 3) {
            case 0:
                candidate = make_points(i + 6);
                break;
            case 1:
                candidate = make_raw(i + 7);
                break;
            case 2:
                candidate = current;
                candidate.cells[i % 3].raw[0] += i + candidate.guard;
                break;
            default:
                candidate = (i & 1) ? make_raw(current.mode + i)
                                    : make_points(current.mode + i + 2);
                break;
        }

        if (row_score(candidate) > row_score(current)) {
            current = candidate;
        } else {
            current.guard += i + current.mode;
        }
        total += row_score(current);
    }

    printf("%d %d %d %d\n", current.mode, current.guard, row_score(current), total);
    return 0;
}
