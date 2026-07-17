#include <stdio.h>

typedef struct {
    int lane[3];
    int scale;
} Wave49Lane;

typedef struct {
    Wave49Lane rows[2];
    int tag;
} Wave49Grid;

static int row_score(Wave49Lane lane) {
    return lane.scale
        + lane.lane[0] * 3
        - lane.lane[1] * 5
        + lane.lane[2] * 7;
}

static int grid_score(Wave49Grid grid) {
    return grid.tag * 11 + row_score(grid.rows[0]) * 13 - row_score(grid.rows[1]) * 17;
}

int main(void) {
    Wave49Grid grid = {
        {
            {{2, 5, 9}, 4},
            {{3, 8, 13}, 6}
        },
        7
    };
    int total = 0;
    int i;

    for (i = 0; i < 9; ++i) {
        Wave49Lane copy = ((grid_score(grid) + i) & 1)
            ? (Wave49Lane){{grid.rows[1].lane[2] + i, grid.rows[0].lane[0] - i, grid.tag + i}, grid.rows[1].scale + i}
            : (Wave49Lane){{grid.rows[0].lane[1] - i, grid.rows[1].lane[0] + i, grid.rows[0].scale + grid.rows[1].scale}, grid.tag - i};
        Wave49Grid candidate = (i & 2)
            ? (Wave49Grid){{grid.rows[1], copy}, grid.tag + row_score(copy)}
            : (Wave49Grid){{copy, grid.rows[0]}, grid.tag - row_score(copy)};
        grid = ((grid_score(candidate) + total) & 3)
            ? candidate
            : (Wave49Grid){{candidate.rows[1], (Wave49Lane){{candidate.tag, total & 31, i - candidate.rows[0].scale}, candidate.rows[0].scale + i}}, candidate.tag ^ i};
        grid.rows[i & 1].lane[(i + 1) % 3] += grid.tag - i;
        total += grid_score(grid);
    }

    printf("%d %d %d %d %d\n", grid.tag, grid.rows[0].scale, grid.rows[1].lane[2], grid_score(grid), total);
    return 0;
}
