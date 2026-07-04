#include <stdio.h>
#include <string.h>

int main(void) {
    char text[24];
    char tail[8];
    char *dash;
    char *beta;
    size_t prefix;
    size_t suffix;
    int cmp;

    strncpy(text, "alpha", sizeof(text));
    text[sizeof(text) - 1] = '\0';
    strncat(text, "-beta", sizeof(text) - strlen(text) - 1);

    memset(tail, '?', sizeof(tail));
    memcpy(tail, text + 6, 4);
    tail[4] = '\0';

    dash = strchr(text, '-');
    beta = strstr(text, "beta");
    prefix = strspn(text, "abcdefghijklmnopqrstuvwxyz");
    suffix = dash ? strcspn(dash + 1, "xyz") : 0U;
    cmp = memcmp(tail, "beta", 4);

    printf("string-span-copy len=%lu dash=%ld beta=%ld prefix=%lu suffix=%lu cmp=%d tail=%s text=%s\n",
           (unsigned long)strlen(text),
           dash ? (long)(dash - text) : -1L,
           beta ? (long)(beta - text) : -1L,
           (unsigned long)prefix,
           (unsigned long)suffix,
           cmp,
           tail,
           text);

    return strlen(text) == 10U && dash == text + 5 && beta == text + 6 &&
                   prefix == 5U && suffix == 4U && cmp == 0 &&
                   strcmp(tail, "beta") == 0 && strcmp(text, "alpha-beta") == 0
               ? 0
               : 1;
}
