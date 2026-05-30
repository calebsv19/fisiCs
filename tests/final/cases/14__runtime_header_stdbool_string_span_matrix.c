#include <stdbool.h>
#include <stdio.h>
#include <string.h>

int main(void) {
    static const char digits[] = "0123456789";
    char tail[8] = {0};
    char text[] = "alpha-42-beta";
    bool ok = false;
    size_t prefix = strcspn(text, "-");
    size_t digit_span = strspn(text + 6, digits);

    memmove(tail, text + 9, 4U);
    tail[4] = '\0';
    ok = (prefix == 5U) && (digit_span == 2U) && (strcmp(tail, "beta") == 0);

    printf(
        "prefix=%zu digits=%zu tail=%s ok=%d\n",
        prefix,
        digit_span,
        tail,
        ok ? 1 : 0);
    return ok ? 0 : 1;
}
