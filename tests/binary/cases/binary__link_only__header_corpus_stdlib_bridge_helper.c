#include <stdlib.h>

int wave26_stdlib_bridge_score(const char *text) {
    char *end = 0;
    long value = strtol(text, &end, 10);

    return (int)labs(value) + (int)(end - text);
}
