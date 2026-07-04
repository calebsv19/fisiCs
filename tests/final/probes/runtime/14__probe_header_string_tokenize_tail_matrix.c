#include <stdio.h>
#include <string.h>

int main(void) {
    char text[] = "alpha::beta:gamma";
    char *state = 0;
    char *tok1 = strtok(text, ":");
    char *tok2 = strtok(0, ":");
    char *tok3 = strtok(0, ":");
    char *tok4 = strtok(0, ":");
    char *tail = tok3 ? strrchr(tok3, 'm') : 0;
    char *pick = tok2 ? strpbrk(tok2, "tx") : 0;
    int len1 = tok1 ? (int)strlen(tok1) : -1;
    int len2 = tok2 ? (int)strlen(tok2) : -1;
    int len3 = tok3 ? (int)strlen(tok3) : -1;
    int tail_pos = tail ? (int)(tail - tok3) : -1;
    int pick_pos = pick ? (int)(pick - tok2) : -1;
    int span = tok1 ? (int)strspn(tok1, "abcdefghijklmnopqrstuvwxyz") : -1;
    int final = tok4 == 0 ? 1 : 0;
    int summary = len1 * 11 + len2 * 7 + len3 * 5 + tail_pos * 3 +
                  pick_pos * 2 + span + final;

    printf("string-token lens=%d,%d,%d tail=%d pick=%d span=%d final=%d summary=%d\n",
           len1,
           len2,
           len3,
           tail_pos,
           pick_pos,
           span,
           final,
           summary);

    return len1 == 5 && len2 == 4 && len3 == 5 && tail_pos == 3 &&
                   pick_pos == 2 && span == 5 && final == 1 &&
                   summary == 127
               ? 0
               : 1;
}
