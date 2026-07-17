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

static struct wave55_factory_result wave55_factory_left(struct wave55_factory_token token, int salt) {
    struct wave55_factory_result out;

    out.total = (long)token.left * 17 + (long)token.right * 3 + salt;
    out.lane = token.left - token.right + salt;
    return out;
}

static struct wave55_factory_result wave55_factory_right(struct wave55_factory_token token, int salt) {
    struct wave55_factory_result out;

    out.total = (long)token.left * 5 - (long)token.right * 7 + salt * 2;
    out.lane = token.left + token.right + salt;
    return out;
}

static wave55_factory_leaf_fn wave55_factory_even_router(int selector) {
    return (selector & 1) == 0 ? wave55_factory_left : wave55_factory_right;
}

static wave55_factory_leaf_fn wave55_factory_odd_router(int selector) {
    return (selector & 1) == 0 ? wave55_factory_right : wave55_factory_left;
}

wave55_factory_router_fn wave55_nested_indirect_factory_contract(int mode) {
    return (mode & 1) == 0 ? wave55_factory_even_router : wave55_factory_odd_router;
}
