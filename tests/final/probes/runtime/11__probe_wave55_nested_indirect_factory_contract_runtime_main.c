#include <stdio.h>

struct wave55_factory_token {
    int left;
    int right;
};

struct wave55_factory_result {
    long total;
    int lane;
};

typedef struct wave55_factory_result (*wave55_factory_leaf_fn)(struct wave55_factory_token token, int salt);
typedef wave55_factory_leaf_fn (*wave55_factory_router_fn)(int selector);

wave55_factory_router_fn wave55_nested_indirect_factory_contract(int mode);

int main(void) {
    struct wave55_factory_token token = {6, 11};
    wave55_factory_router_fn router = wave55_nested_indirect_factory_contract(5);
    wave55_factory_leaf_fn leaf = router(8);
    struct wave55_factory_result got = leaf(token, 7);

    printf("%ld %d\n", got.total, got.lane);
    return 0;
}
