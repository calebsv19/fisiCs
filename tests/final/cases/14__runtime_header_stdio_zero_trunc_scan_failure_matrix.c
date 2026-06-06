#include <stdio.h>

int main(void) {
    char tiny[5];
    char word[8] = {0};
    int value = 0;
    int count = -1;
    char ch = 0;
    int bad = 77;
    int left = 0;
    int right = 0;
    int used = -1;

    int n0 = snprintf(NULL, 0, "[%.*s:%04d]", 3, "abcdef", 7);
    int n1 = snprintf(tiny, sizeof(tiny), "%s", "abcdef");
    int m1 = sscanf("abc123:Z", "%3[a-z]%3d:%c%n", word, &value, &ch, &count);
    int m2 = sscanf("xx", "%d", &bad);
    int m3 = sscanf("12 skip 34", "%d %*s %d%n", &left, &right, &used);
    int summary = n0 + n1 + m1 + count + m2 + bad + m3 + used + left + right + value + (int)ch;

    printf("stdio-zero-scan n0=%d n1=%d tiny=%s m1=%d word=%s value=%d ch=%c count=%d m2=%d bad=%d m3=%d used=%d summary=%d\n",
           n0,
           n1,
           tiny,
           m1,
           word,
           value,
           ch,
           count,
           m2,
           bad,
           m3,
           used,
           summary);

    return n0 == 10 && n1 == 6 && tiny[0] == 'a' && tiny[3] == 'd' &&
                   tiny[4] == '\0' && m1 == 3 && value == 123 && ch == 'Z' &&
                   count == 8 && m2 == 0 && bad == 77 && m3 == 2 &&
                   left == 12 && right == 34 && used == 10 && summary == 375
               ? 0
               : 1;
}
