// Ensure aggregate zero-init uses semantic size/align (with explicit alignas).
#include <stdalign.h>

struct Agg {
    alignas(32) int x;
};

int main(void) {
    struct Agg a = {0};
    return a.x;
}
