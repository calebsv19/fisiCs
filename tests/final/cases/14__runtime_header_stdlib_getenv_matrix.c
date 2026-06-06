#include <stdio.h>
#include <stdlib.h>

static int text_score(const char *text) {
    int total = 0;
    int i = 0;

    if (!text) {
        return -1;
    }
    while (text[i] != '\0') {
        total += (unsigned char)text[i] * (i + 1);
        i++;
    }
    return total + i * 31;
}

int main(void) {
    const char *alpha = getenv("FISICS_FINAL_GETENV_ALPHA");
    const char *empty = getenv("FISICS_FINAL_GETENV_EMPTY");
    const char *missing = getenv("FISICS_FINAL_GETENV_MISSING");
    int alpha_score = text_score(alpha);
    int empty_score = text_score(empty);
    int missing_ok = missing == 0;
    int alpha_ok = alpha != 0 && alpha[0] == 'h' && alpha[1] == 'e' &&
                   alpha[2] == 'a' && alpha[3] == 'd' && alpha[4] == 'e' &&
                   alpha[5] == 'r' && alpha[6] == '-' && alpha[7] == '3' &&
                   alpha[8] == '2' && alpha[9] == '2' && alpha[10] == '\0';
    int empty_ok = empty != 0 && empty[0] == '\0';
    int summary = alpha_score + empty_score + missing_ok * 17 + alpha_ok * 19 +
                  empty_ok * 23;

    printf("stdlib-getenv alpha=%d empty=%d missing=%d score=%d empty_score=%d summary=%d\n",
           alpha_ok,
           empty_ok,
           missing_ok,
           alpha_score,
           empty_score,
           summary);

    return alpha_ok && empty_ok && missing_ok && alpha_score == 4169 &&
                   empty_score == 0 && summary == 4228
               ? 0
               : 1;
}
