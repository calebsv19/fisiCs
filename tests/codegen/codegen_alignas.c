// Check alignas/_Alignas propagate to IR alignment.
#include <stdalign.h>

alignas(128) int g_aligned = 3;

alignas(32) struct A {
    int x;
};

int main(void) {
    alignas(16) struct A a = { .x = g_aligned };
    alignas(32) int local = a.x;
    return local + a.x;
}
