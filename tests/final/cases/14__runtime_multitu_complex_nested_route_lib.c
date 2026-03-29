typedef struct {
    _Complex double value;
    int code;
} inner_node;

typedef struct {
    int bias;
    inner_node inner;
} outer_node;

outer_node route_outer(outer_node in) {
    outer_node out = in;
    out.inner.value = in.inner.value - (_Complex double)0.5;
    out.inner.code = in.inner.code + in.bias;
    return out;
}
