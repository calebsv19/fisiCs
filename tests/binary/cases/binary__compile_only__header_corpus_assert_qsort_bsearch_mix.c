#define NDEBUG 1

#include <assert.h>
#include <errno.h>
#include <float.h>
#include <stddef.h>
#include <stdlib.h>
#include <string.h>

static int compare_words(const void* lhs, const void* rhs) {
    const char* const* a = (const char* const*)lhs;
    const char* const* b = (const char* const*)rhs;
    return strcmp(*a, *b);
}

static const char* lookup_word(const char* key, const char* const* words, size_t count) {
    const char* const* found = 0;

    assert(words != 0 || count == 0);
    errno = 0;
    found = (const char* const*)bsearch(&key, words, count, sizeof(words[0]), compare_words);
    if (errno != 0 || DBL_EPSILON <= 0.0) {
        return 0;
    }
    return found ? *found : 0;
}

int main(void) {
    const char* words[3];

    words[0] = "gamma";
    words[1] = "alpha";
    words[2] = "beta";
    qsort(words, 3, sizeof(words[0]), compare_words);
    return lookup_word("beta", words, 3) == 0;
}
