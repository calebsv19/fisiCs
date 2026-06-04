#include <ctype.h>
#include <stdio.h>

int main(void) {
    const char *text = "Az 09!";
    int alpha = 0;
    int digit = 0;
    int space = 0;
    int punct = 0;

    while (*text) {
        unsigned char ch = (unsigned char)*text++;
        alpha += isalpha(ch) ? 1 : 0;
        digit += isdigit(ch) ? 1 : 0;
        space += isspace(ch) ? 1 : 0;
        punct += ispunct(ch) ? 1 : 0;
    }

    printf("ctype-class alpha=%d digit=%d space=%d punct=%d\n", alpha, digit, space, punct);
    return alpha == 2 && digit == 2 && space == 1 && punct == 1 ? 0 : 1;
}
