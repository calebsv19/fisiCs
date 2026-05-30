#include <stdbool.h>
#include <string.h>

bool header_corpus_wave5_same_prefix(const char *left, const char *right, size_t count) {
    if (!left || !right) {
        return false;
    }

    return memcmp(left, right, count) == 0;
}
