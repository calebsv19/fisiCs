#define NDEBUG 1

#include <assert.h>
#include <ctype.h>
#include <errno.h>
#include <float.h>
#include <stddef.h>

struct CtypeFloatSpan {
    const char* begin;
    size_t len;
};

static int score_span(struct CtypeFloatSpan span) {
    size_t i = 0;
    int score = 0;

    assert(span.begin != 0 || span.len == 0);

    for (i = 0; i < span.len; ++i) {
        unsigned char ch = (unsigned char)span.begin[i];

        if (isalpha(ch)) {
            score += tolower(ch) - 'a' + 1;
        } else if (isdigit(ch)) {
            score += ch - '0';
        } else if (isspace(ch)) {
            score += 1;
        }
    }

    if (DBL_EPSILON > 0.0) {
        score += 1;
    }

    errno = 0;
    return score + errno;
}

int main(void) {
    struct CtypeFloatSpan span;

    span.begin = "A9 z";
    span.len = 4;
    return score_span(span) < 0;
}
