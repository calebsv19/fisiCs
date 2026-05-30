#define NDEBUG 1

#include <assert.h>

int header_corpus_errno_digit_score(const char* raw);

int main(void) {
    assert(sizeof(int) >= 2);
    return header_corpus_errno_digit_score("7");
}
