#include <assert.h>

int wave28_assert_control_surface(int value) {
    assert(value >= 0);
    assert((value == 0) || (value > 0));
    return value;
}

int main(void) {
    return 0;
}
