#include <assert.h>

int wave28_assert_bridge_score(int value) {
    assert(value > 0);
    return value * 2;
}
