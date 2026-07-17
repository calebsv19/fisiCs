#include <stdio.h>

typedef union {
    struct {
        int x;
        int y;
    } point;
    int raw[2];
} Wave43SwitchCell;

typedef struct {
    int mode;
    Wave43SwitchCell cells[4];
    int guard[2];
} Wave43SwitchRow;

static Wave43SwitchRow make_point_row(int seed) {
    Wave43SwitchRow row;
    int i;
    row.mode = 1;
    row.guard[0] = seed + 50;
    row.guard[1] = seed + 80;
    for (i = 0; i < 4; ++i) {
        row.cells[i].point.x = seed + i * 3;
        row.cells[i].point.y = seed - i * 2;
    }
    return row;
}

static Wave43SwitchRow make_raw_row(int seed) {
    Wave43SwitchRow row;
    int i;
    row.mode = 2;
    row.guard[0] = seed + 70;
    row.guard[1] = seed + 90;
    for (i = 0; i < 4; ++i) {
        row.cells[i].raw[0] = seed * 2 + i;
        row.cells[i].raw[1] = seed * 3 - i * 2;
    }
    return row;
}

static int row_score(Wave43SwitchRow row) {
    int total = row.mode + row.guard[0] - row.guard[1];
    int i;
    for (i = 0; i < 4; ++i) {
        if (row.mode == 1) {
            total += row.cells[i].point.x * (i + 1) + row.cells[i].point.y;
        } else {
            total += row.cells[i].raw[0] - row.cells[i].raw[1] * (i + 2);
        }
    }
    return total;
}

int main(void) {
    Wave43SwitchRow row = make_point_row(5);
    int total = 0;
    int i;

    for (i = 0; i < 9; ++i) {
        Wave43SwitchRow candidate = row;
        switch ((row_score(row) + i) % 5) {
            case 0:
                candidate = make_point_row(i + 8);
                break;
            case 1:
                candidate = make_raw_row(i + 9);
                break;
            case 2:
                candidate.cells[i % 4].raw[0] += row.guard[0] + i;
                candidate.guard[1] -= i;
                break;
            case 3:
                candidate = (i & 1) ? make_raw_row(row.mode + i + 3)
                                    : make_point_row(row.guard[0] - 40 + i);
                break;
            default:
                row.guard[0] += i + row.mode;
                total += row_score(row);
                continue;
        }

        if (row_score(candidate) > row_score(row) || (i % 3) == 0) {
            row = candidate;
        } else {
            row.guard[i & 1] += i + row.mode;
        }
        total += row_score(row);
    }

    printf("%d %d %d %d\n", row.mode, row.guard[0], row_score(row), total);
    return 0;
}
