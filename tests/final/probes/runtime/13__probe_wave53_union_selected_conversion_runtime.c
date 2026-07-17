#include <stdio.h>

typedef union {
    int whole;
    float ratio;
} Wave53Payload;

typedef struct {
    int tag;
    Wave53Payload payload;
    int trail;
} Wave53Token;

static Wave53Token convert(Wave53Token token, int choose_ratio) {
    Wave53Token out = token;

    if (choose_ratio) {
        out.tag = 1;
        out.payload.ratio = (float)(token.payload.whole + token.trail) / 4.0f;
        out.trail += (int)out.payload.ratio;
    } else {
        out.tag = 0;
        out.payload.whole = (int)(token.payload.ratio * 6.0f) - token.trail;
        out.trail += out.payload.whole & 7;
    }
    return out;
}

static int token_score(Wave53Token token) {
    return token.tag ? (int)(token.payload.ratio * 10.0f) + token.trail * 13
                     : token.payload.whole * 5 - token.trail * 7;
}

int main(void) {
    Wave53Token token = {0, {19}, 4};
    int total = 0;
    int i;

    for (i = 0; i < 10; ++i) {
        token = convert(token, token.tag == 0);
        total += token_score(token) + i;
    }

    printf("%d %d %d %d\n", token.tag,
           token.tag ? (int)(token.payload.ratio * 100.0f) : token.payload.whole,
           token.trail, total);
    return 0;
}
