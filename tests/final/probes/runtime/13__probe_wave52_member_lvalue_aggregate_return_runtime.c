#include <stdio.h>

typedef struct {
    int left;
    int right;
} Wave52Pair;

typedef struct {
    Wave52Pair pair;
    int slots[2];
    int tag;
} Wave52Record;

static Wave52Record rewrite(Wave52Record record, int step) {
    int *selected = (step & 1) ? &record.pair.right : &record.slots[step & 1];

    *selected += record.tag + step;
    record.pair.left += record.slots[0] - step;
    record.tag = record.pair.right - record.slots[1] + step;
    return record;
}

static int record_score(Wave52Record record) {
    return record.pair.left * 5 - record.pair.right * 7
        + record.slots[0] * 11 - record.slots[1] * 13 + record.tag * 17;
}

int main(void) {
    Wave52Record record = {{3, 8}, {13, 21}, 34};
    Wave52Record saved = {{2, 5}, {7, 11}, 17};
    int total = 0;
    int i;

    for (i = 0; i < 10; ++i) {
        Wave52Record next = rewrite(record, i);
        if ((record_score(next) + i) & 1) {
            next = rewrite(saved, i + 2);
        }
        saved = record;
        record = next;
        total += record_score(record);
    }

    printf("%d %d %d %d %d\n", record.pair.left, record.pair.right,
           saved.slots[1], record_score(record), total);
    return 0;
}
