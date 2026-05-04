#include <stdio.h>

union Payload {
    char text[10];
    struct {
        char c0;
        char c1;
        char c2;
        char c3;
        char c4;
        char c5;
        char c6;
        char c7;
        char c8;
        char c9;
    } chars;
};

struct Bucket {
    union Payload cells[2][2];
    int tag;
};

struct BucketMatrix {
    struct Bucket buckets[2][2];
};

static unsigned checksum(const struct Bucket buckets[2][2]) {
    unsigned acc = 0;

    for (int x = 0; x < 2; ++x) {
        for (int y = 0; y < 2; ++y) {
            for (int r = 0; r < 2; ++r) {
                for (int c = 0; c < 2; ++c) {
                    const union Payload *p = &buckets[x][y].cells[r][c];
                    acc = acc * 67u +
                          (unsigned char)p->text[0] * 13u +
                          (unsigned char)p->text[1] * 11u +
                          (unsigned char)p->text[2] * 7u +
                          (unsigned char)p->text[3] * 5u +
                          (unsigned)buckets[x][y].tag;
                }
            }
        }
    }
    return acc;
}

int main(void) {
    struct BucketMatrix matrix = {
        .buckets[0][0].cells[0][0].text = "ab",
        .buckets[0][0].cells[0][1].text = "cd",
        .buckets[0][0].tag = 2,
        .buckets[0][1].cells[1][0].chars = { 'm', 'n', 'o', 'p', 0, 0, 0, 0, 0, 0 },
        .buckets[0][1].cells[1][0].text = "io",
        .buckets[0][1].cells[1][0].chars.c2 = '!',
        .buckets[0][1].tag = 3,
        .buckets[1][0].cells[0][1].text = "jk",
        .buckets[1][0].cells[0][1].chars.c6 = '?',
        .buckets[1][0].cells[0][1].text = "rs",
        .buckets[1][0].cells[0][1].chars.c0 = 'A',
        .buckets[1][0].tag = 5,
        .buckets[1][1].cells[1][1].chars = { 'q', 'r', 's', 0, 0, 0, 0, 0, 0, 0 },
        .buckets[1][1].cells[1][1].text = "lm",
        .buckets[1][1].cells[1][1].chars.c1 = 'p',
        .buckets[1][1].tag = 7,
    };

    printf("%d %d %d %u\n",
           matrix.buckets[0][0].cells[0][1].text[1],
           matrix.buckets[0][1].cells[1][0].text[2],
           matrix.buckets[1][1].cells[1][1].text[1],
           checksum(matrix.buckets));
    return 0;
}
