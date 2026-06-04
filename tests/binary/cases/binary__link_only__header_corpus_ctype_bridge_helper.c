#include <ctype.h>

int wave24_ctype_bridge_score(const char *text) {
    int score = 0;
    while (*text) {
        unsigned char ch = (unsigned char)*text++;
        score += isalpha(ch) ? 3 : 0;
        score += isdigit(ch) ? 5 : 0;
        score += isspace(ch) ? 7 : 0;
        score += ispunct(ch) ? 11 : 0;
    }
    return score;
}
