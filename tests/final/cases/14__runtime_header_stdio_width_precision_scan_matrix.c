#include <stdio.h>

int main(void) {
    char trunc[8];
    char wide[24];
    char word[8];
    char tail[8];
    int left = -1;
    unsigned hex = 0;
    long span = 0;
    int used = -1;

    int n1 = snprintf(trunc, sizeof(trunc), "%08d:%s", 123, "tail");
    int n2 = snprintf(wide, sizeof(wide), "%-6.3s|%+05d|%#x", "abcdef", 42, 26);
    int matched = sscanf("alpha 0x2a 17 beta", "%7s %x %ld %n%7s", word, &hex, &span, &used, tail);

    printf("stdio-width n1=%d trunc=%s n2=%d wide=%s matched=%d word=%s hex=%u span=%ld used=%d tail=%s\n",
           n1,
           trunc,
           n2,
           wide,
           matched,
           word,
           hex,
           span,
           used,
           tail);

    return n1 == 13 && trunc[0] == '0' && trunc[6] == '2' && trunc[7] == '\0' &&
                   n2 == 17 && matched == 4 && word[0] == 'a' && word[4] == 'a' &&
                   hex == 42U && span == 17L && used == 14 && tail[0] == 'b'
               ? 0
               : 1;
}
