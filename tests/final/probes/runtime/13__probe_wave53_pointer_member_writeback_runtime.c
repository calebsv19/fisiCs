#include <stdio.h>

typedef struct {
    int x;
    int y;
} Wave53Point;

typedef struct {
    Wave53Point points[2];
    int cursor;
} Wave53Table;

static Wave53Table rewrite(Wave53Table table, int step) {
    Wave53Table copy = table;
    Wave53Point *point = &copy.points[(copy.cursor + step) & 1];
    int *field = (step & 1) ? &point->y : &point->x;

    *field += copy.cursor + step;
    copy.points[(copy.cursor + 1) & 1].x -= point->y - step;
    copy.cursor = (copy.cursor + point->x + step) & 7;
    return copy;
}

static int table_score(Wave53Table table) {
    return table.points[0].x * 3 - table.points[0].y * 5
        + table.points[1].x * 7 - table.points[1].y * 11 + table.cursor * 13;
}

int main(void) {
    Wave53Table table = {{{3, 5}, {8, 13}}, 2};
    Wave53Table saved = table;
    int total = 0;
    int i;

    for (i = 0; i < 11; ++i) {
        Wave53Table next = rewrite(table, i);
        if ((table_score(next) + i) & 1) {
            next = rewrite(saved, i + 2);
        }
        saved = table;
        table = next;
        total += table_score(table);
    }

    printf("%d %d %d %d %d\n", table.points[0].x, table.points[0].y,
           saved.points[1].x, table.cursor, total);
    return 0;
}
