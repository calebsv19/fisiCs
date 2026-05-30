#include <float.h>

int main(void) {
    int rounds = FLT_ROUNDS;
    return rounds < -1 ? 1 : 0;
}
