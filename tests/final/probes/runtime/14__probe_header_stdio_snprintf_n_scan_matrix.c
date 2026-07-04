#include <stdio.h>
#include <string.h>

int main(void) {
    char out[32];
    char tiny[6];
    char word[8] = {0};
    int value = -1;
    int used = -1;
    int trunc;
    int n;
    int matched;
    char tag = '\0';

    n = snprintf(out, sizeof(out), "%s:%04d:%c", "ax", 23, 'Q');
    matched = sscanf(out, "%7[^:]:%d:%c%n", word, &value, &tag, &used);
    trunc = snprintf(tiny, sizeof(tiny), "%d-%d", 123, 45);

    printf("stdio-scan n=%d matched=%d used=%d trunc=%d word=%s value=%d tag=%c tiny=%s\n",
           n,
           matched,
           used,
           trunc,
           word,
           value,
           tag,
           tiny);

    return n == 9 && matched == 3 && used == 9 && trunc == 6 &&
                   strcmp(word, "ax") == 0 && value == 23 && tag == 'Q' &&
                   strcmp(tiny, "123-4") == 0
               ? 0
               : 1;
}
