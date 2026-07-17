#include <stdio.h>

typedef struct {
    int lane[3];
    int stamp;
} Wave52Cell;

typedef struct {
    Wave52Cell cells[2];
    int mode;
} Wave52Frame;

static Wave52Frame make_frame(int seed, int mode) {
    Wave52Frame frame = {{{seed + 1, seed * 2, seed - 3, seed * 5},
                          {seed + 7, seed - 4, seed * 3, seed * 11}}, mode};
    return frame;
}

static int frame_score(Wave52Frame frame) {
    return frame.mode * 19
        + frame.cells[0].lane[0] * 3 - frame.cells[0].lane[1] * 5
        + frame.cells[0].lane[2] * 7 + frame.cells[0].stamp * 11
        + frame.cells[1].lane[0] * 13 - frame.cells[1].lane[1] * 17
        + frame.cells[1].lane[2] * 23 + frame.cells[1].stamp * 29;
}

int main(void) {
    Wave52Frame frame = make_frame(4, 0);
    Wave52Frame shadow = make_frame(7, 1);
    int total = 0;
    int i;

    for (i = 0; i < 11; ++i) {
        Wave52Frame next;

        switch ((frame_score(frame) + total + i) & 3) {
            case 0:
                next = make_frame(i + 5, 2);
                next.cells[0].lane[1] += shadow.cells[1].stamp;
                break;
            case 1:
                next = shadow;
                next.cells[1].lane[i % 3] -= frame.cells[0].lane[(i + 1) % 3];
                break;
            case 2:
                next = frame;
                next.cells[0].stamp += i + shadow.mode;
                break;
            default:
                next = make_frame(frame.cells[1].lane[0] - i, frame.mode + 3);
                next.cells[1] = shadow.cells[0];
                break;
        }
        shadow = frame;
        frame = next;
        total += frame_score(frame);
    }

    printf("%d %d %d %d %d\n", frame.mode, frame.cells[0].stamp,
           shadow.cells[1].lane[2], frame_score(frame), total);
    return 0;
}
