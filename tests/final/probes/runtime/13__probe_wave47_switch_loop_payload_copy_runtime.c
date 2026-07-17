#include <stdio.h>

typedef struct {
    int tag;
    int kind;
    union {
        int words[3];
        struct {
            int x;
            int y;
        } pair;
    } data;
    int tail;
} Wave47Payload;

static Wave47Payload make_words(int seed, int step) {
    Wave47Payload payload;
    payload.tag = seed + step;
    payload.kind = 0;
    payload.data.words[0] = seed * 2 + step;
    payload.data.words[1] = seed - step * 3;
    payload.data.words[2] = seed + step * step;
    payload.tail = seed * 5 - step;
    return payload;
}

static Wave47Payload make_pair(int seed, int step) {
    Wave47Payload payload;
    payload.tag = seed - step;
    payload.kind = 1;
    payload.data.pair.x = seed * 7 + step;
    payload.data.pair.y = seed * 3 - step * 2;
    payload.tail = seed + step * 11;
    return payload;
}

static int score_payload(Wave47Payload payload) {
    int total = payload.tag * 3 + payload.kind * 17 + payload.tail;
    if (payload.kind) {
        total += payload.data.pair.x * 5 - payload.data.pair.y;
    } else {
        total += payload.data.words[0] - payload.data.words[1] * 2 + payload.data.words[2] * 4;
    }
    return total;
}

int main(void) {
    Wave47Payload current = make_words(4, 1);
    int total = 0;
    int i;

    for (i = 0; i < 12; ++i) {
        Wave47Payload candidate;
        switch ((i + current.tail) & 3) {
            case 0:
                candidate = make_words(i + 3, current.tag & 7);
                break;
            case 1:
                candidate = make_pair(i + 5, current.tail & 5);
                break;
            case 2:
                candidate = current;
                candidate.tail += i * 2;
                candidate.kind = 0;
                candidate.data.words[0] += candidate.tag - i;
                break;
            default:
                candidate = make_pair(current.tag + i, i & 3);
                candidate.data.pair.y += current.tail - i;
                break;
        }
        if ((score_payload(candidate) ^ total) & 1) {
            current = candidate;
        } else {
            current.tail += candidate.tag - i;
            current.tag ^= i & 3;
        }
        total += score_payload(current);
    }

    printf("%d %d %d %d\n", current.tag, current.tail, score_payload(current), total);
    return 0;
}
