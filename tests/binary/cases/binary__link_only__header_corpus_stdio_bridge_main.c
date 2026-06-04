#include <stddef.h>

int wave25_stdio_bridge_score(const char *label, int value);

int main(void) {
    return wave25_stdio_bridge_score("io", 17) == 165 ? 0 : 1;
}
