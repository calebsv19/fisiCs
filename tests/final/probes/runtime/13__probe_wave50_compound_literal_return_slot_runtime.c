#include <stdio.h>

typedef struct {
    int words[4];
    int bias;
} Wave50LiteralRow;

typedef struct {
    Wave50LiteralRow rows[2];
    int tag;
} Wave50LiteralFrame;

static int row_score(Wave50LiteralRow row) {
    return row.bias * 3
        + row.words[0] * 5
        - row.words[1] * 7
        + row.words[2] * 11
        - row.words[3] * 13;
}

static int frame_score(Wave50LiteralFrame frame) {
    return frame.tag * 17 + row_score(frame.rows[0]) * 19 - row_score(frame.rows[1]) * 23;
}

static Wave50LiteralFrame make_frame(int seed, int step) {
    Wave50LiteralFrame frame = {
        {
            {{seed + step, seed * 2 - step, seed + 3, step + 5}, seed - step},
            {{seed ^ (step + 7), seed + step * 3, seed * 4 - step, seed - 2}, seed + step}
        },
        seed * 5 + step
    };
    return frame;
}

static Wave50LiteralFrame rewrite(Wave50LiteralFrame frame, int step) {
    Wave50LiteralRow keep = frame.rows[step & 1];
    Wave50LiteralFrame out = (step & 2)
        ? (Wave50LiteralFrame){{keep, (Wave50LiteralRow){{frame.tag, row_score(keep), step, keep.bias - step}, frame.tag + step}}, frame.tag - step}
        : (Wave50LiteralFrame){{(Wave50LiteralRow){{keep.words[3], keep.words[0] + step, keep.words[1] - step, keep.words[2]}, keep.bias + step}, frame.rows[(step + 1) & 1]}, frame.tag + row_score(keep)};
    out.rows[(step + 1) & 1].words[step % 4] += frame_score(frame) & 31;
    return out;
}

int main(void) {
    Wave50LiteralFrame frame = make_frame(3, 1);
    int total = 0;
    int i;

    for (i = 0; i < 10; ++i) {
        Wave50LiteralFrame local = rewrite(make_frame(i + 4, frame.tag & 3), i);
        Wave50LiteralFrame candidate = ((frame_score(frame) + i) & 1)
            ? rewrite(frame, i + 1)
            : (Wave50LiteralFrame){{local.rows[1], frame.rows[0]}, local.tag + frame.tag};
        frame = ((frame_score(candidate) ^ total) & 4)
            ? candidate
            : rewrite(local, i + 2);
        total += frame_score(frame);
    }

    printf("%d %d %d %d %d\n", frame.tag, frame.rows[0].bias, frame.rows[1].words[2], frame_score(frame), total);
    return 0;
}
