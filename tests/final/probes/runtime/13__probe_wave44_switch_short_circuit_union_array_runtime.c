#include <stdio.h>

typedef union {
    struct {
        int x;
        int y;
    } point;
    int raw[2];
} Wave44SwitchCell;

typedef struct {
    int kind;
    Wave44SwitchCell cells[3];
    int tail;
} Wave44SwitchRow;

static Wave44SwitchRow make_point_row(int seed) {
    Wave44SwitchRow row;
    int i;
    row.kind = 1;
    row.tail = seed + 90;
    for (i = 0; i < 3; ++i) {
        row.cells[i].point.x = seed + i * 4;
        row.cells[i].point.y = seed * 2 - i;
    }
    return row;
}

static Wave44SwitchRow make_raw_row(int seed) {
    Wave44SwitchRow row;
    int i;
    row.kind = 2;
    row.tail = seed + 110;
    for (i = 0; i < 3; ++i) {
        row.cells[i].raw[0] = seed * 3 + i;
        row.cells[i].raw[1] = seed - i * 5;
    }
    return row;
}

static int row_score(Wave44SwitchRow row) {
    int total = row.kind + row.tail;
    int i;
    for (i = 0; i < 3; ++i) {
        if (row.kind == 1) {
            total += row.cells[i].point.x * (i + 2) - row.cells[i].point.y;
        } else {
            total += row.cells[i].raw[0] - row.cells[i].raw[1] * (i + 3);
        }
    }
    return total;
}

int main(void) {
    Wave44SwitchRow row = make_point_row(4);
    int total = 0;
    int i;

    for (i = 0; i < 11; ++i) {
        Wave44SwitchRow next = row;
        int key = (row_score(row) + i) % 6;

        switch (key) {
            case 0:
                next = make_raw_row(i + row.kind + 5);
                break;
            case 1:
                next = make_point_row(row.tail % 9 + i);
                next.cells[i % 3].point.y += row.kind + i;
                break;
            case 2:
                if ((row.kind == 1 && row.cells[0].point.x < row.tail) ||
                    (row.kind == 2 && row.cells[1].raw[0] > row.cells[2].raw[1])) {
                    next.tail += i + row.kind;
                    next.cells[(i + 1) % 3].raw[0] += row.tail;
                }
                break;
            case 3:
                row.tail += i;
                total += row_score(row);
                continue;
            default:
                next = (i & 1) ? make_raw_row(i + 7) : make_point_row(i + 6);
                break;
        }

        row = (row_score(next) >= row_score(row) - i) ? next : row;
        total += row_score(row);
    }

    printf("%d %d %d %d\n", row.kind, row.tail, row_score(row), total);
    return 0;
}
