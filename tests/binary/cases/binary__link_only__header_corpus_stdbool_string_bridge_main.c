#include <stdbool.h>
#include <stddef.h>

bool header_corpus_wave5_same_prefix(const char *left, const char *right, size_t count);

int main(void) {
    static const char left[] = "header-corpus";
    static const char right[] = "header-capture";

    return header_corpus_wave5_same_prefix(left, right, 6U) ? 0 : 1;
}
