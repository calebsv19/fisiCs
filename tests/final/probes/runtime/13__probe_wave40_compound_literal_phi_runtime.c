#include <stdio.h>

typedef struct {
    int lane[4];
    int tag;
} Wave40Cell;

static Wave40Cell make_cell(int base) {
    return (Wave40Cell){{ base + 1, base + 3, base * 2, base * 3 }, base * 11};
}

static int score_cell(Wave40Cell cell) {
    return cell.lane[0] * 5 + cell.lane[1] * 3 - cell.lane[2] +
           cell.lane[3] * 2 + cell.tag;
}

int main(void) {
    Wave40Cell current = make_cell(3);
    int total = score_cell(current);
    int i;

    for (i = 0; i < 6; ++i) {
        Wave40Cell alt = (i & 1)
            ? make_cell(i + 5)
            : (Wave40Cell){{ i + 2, i * 3 + 1, i + 9, i * i + 4 }, i * 13 + 7};
        Wave40Cell picked = (score_cell(alt) > score_cell(current)) ? alt : current;
        int idx = (picked.tag + i) & 3;

        picked.lane[idx] += score_cell(current) % 17;
        current = ((picked.lane[(idx + 1) & 3] + i) & 1) ? picked : alt;
        total += score_cell(current) + current.lane[idx];
    }

    printf("%d %d %d %d\n", current.tag, current.lane[0], current.lane[3], total);
    return 0;
}
