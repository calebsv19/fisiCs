#include <ctype.h>
#include <errno.h>
#include <float.h>
#include <stdio.h>
#include <stdlib.h>

int main(void) {
    const char* raw = " \t-12z";
    char* end = 0;
    long value = 0;
    int lead_space = 0;
    int tab_space = 0;
    int tail_alpha = 0;
    int upper = 0;

    errno = 0;
    value = strtol(raw, &end, 10);
    lead_space = isspace((unsigned char)raw[0]) ? 1 : 0;
    tab_space = isspace((unsigned char)raw[1]) ? 1 : 0;
    tail_alpha = (end && isalpha((unsigned char)*end)) ? 1 : 0;
    upper = (end ? toupper((unsigned char)*end) : 0);

    if (value != -12L || errno != 0 || !end || *end != 'z') {
        return 1;
    }
    if (!lead_space || !tab_space || !tail_alpha || upper != 'Z') {
        return 2;
    }
    if (DBL_EPSILON <= 0.0) {
        return 3;
    }

    printf(
        "value=%ld tail=%c upper=%c epsilon=%d errno=%d\n",
        value,
        *end,
        upper,
        DBL_EPSILON > 0.0 ? 1 : 0,
        errno);
    return 0;
}
