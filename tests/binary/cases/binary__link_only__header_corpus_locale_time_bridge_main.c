#include <locale.h>
#include <stddef.h>

size_t header_corpus_format_calendar(char* out, size_t cap);

int main(void) {
    struct lconv* conv = localeconv();
    if (conv == 0) {
        return 1;
    }
    return header_corpus_format_calendar(0, 0) > 0;
}
