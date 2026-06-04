#include <stddef.h>
#include <string.h>

size_t wave23_string_bridge_score(char *dst, size_t cap, const char *lhs, const char *rhs) {
    size_t lhs_len = strlen(lhs);
    size_t rhs_len = strlen(rhs);

    memset(dst, 0, cap);
    memcpy(dst, lhs, lhs_len);
    dst[lhs_len] = ':';
    memcpy(dst + lhs_len + 1, rhs, rhs_len + 1);
    return strlen(dst) + (size_t)(strstr(dst, rhs) != NULL) + (size_t)(strchr(dst, ':') != NULL) + (size_t)(memcmp(dst, lhs, lhs_len) == 0);
}
