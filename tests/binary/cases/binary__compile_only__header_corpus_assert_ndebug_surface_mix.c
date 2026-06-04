#define NDEBUG
#include <assert.h>

int wave28_assert_ndebug_surface(int value) {
    assert(++value == 100);
    return value;
}

int main(void) {
    return wave28_assert_ndebug_surface(4) == 4 ? 0 : 1;
}
