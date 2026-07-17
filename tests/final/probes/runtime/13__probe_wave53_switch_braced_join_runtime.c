#include <stdio.h>

typedef struct {
    int lane[2];
    int stamp;
} Wave53Cell;

typedef struct {
    Wave53Cell cell;
    int mode;
} Wave53Frame;

static Wave53Frame make_frame(int seed, int mode) {
    Wave53Frame frame = {{{seed + 1, seed * 2 - 3}, seed * 5 + 7}, mode};
    return frame;
}

static int frame_score(Wave53Frame frame) {
    return frame.cell.lane[0] * 3 - frame.cell.lane[1] * 5
        + frame.cell.stamp * 7 + frame.mode * 11;
}

int main(void) {
    Wave53Frame frame = {{{3, 5}, 8}, 1};
    Wave53Frame saved = {{{13, 21}, 34}, 2};
    int total = 0;
    int i;

    for (i = 0; i < 12; ++i) {
        Wave53Frame next;

        switch ((frame_score(frame) + saved.mode + i) & 3) {
            case 0:
                next = (Wave53Frame){{{i + 2, i * 3 + 1}, i * 5 + 9}, 3};
                break;
            case 1:
                next = frame;
                next.cell.lane[0] += saved.cell.stamp - i;
                break;
            case 2:
                next = saved;
                next.cell.lane[1] -= frame.mode + i;
                break;
            default:
                next = make_frame(i + frame.mode, saved.mode + 1);
                break;
        }
        saved = frame;
        frame = next;
        total += frame_score(frame);
    }

    printf("%d %d %d %d %d\n", frame.cell.lane[0], frame.cell.lane[1],
           saved.cell.stamp, frame.mode, total);
    return 0;
}
