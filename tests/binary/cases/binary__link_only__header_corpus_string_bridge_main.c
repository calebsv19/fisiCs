#include <stddef.h>

size_t wave23_string_bridge_score(char *dst, size_t cap, const char *lhs, const char *rhs);

int main(void) {
    char out[24];
    size_t score = wave23_string_bridge_score(out, sizeof(out), "left", "right");
    return score == (size_t)15 ? 0 : 1;
}
