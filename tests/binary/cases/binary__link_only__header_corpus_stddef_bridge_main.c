#include <stddef.h>

size_t wave22_stddef_bridge_layout_score(void);

int main(void) {
    size_t score = wave22_stddef_bridge_layout_score();
    return score == (size_t)63 ? 0 : 1;
}
