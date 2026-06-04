#include <ctype.h>

int wave24_ctype_class_surface(int ch) {
    int score = 0;
    score += isalpha(ch) ? 1 : 0;
    score += isdigit(ch) ? 2 : 0;
    score += isspace(ch) ? 4 : 0;
    score += isxdigit(ch) ? 8 : 0;
    return score;
}
