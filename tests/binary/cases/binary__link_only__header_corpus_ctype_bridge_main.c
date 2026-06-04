#include <stddef.h>

int wave24_ctype_bridge_score(const char *text);

int main(void) {
    return wave24_ctype_bridge_score("Az 19!") == 34 ? 0 : 1;
}
