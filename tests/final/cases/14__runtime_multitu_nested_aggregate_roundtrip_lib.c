struct InnerPair {
    int a;
    int b;
};

struct OuterNode {
    struct InnerPair inner;
    long long tail;
};

struct OuterNode abi_outer_step(struct OuterNode node, int mul) {
    struct OuterNode out;
    out.inner.a = node.inner.a * mul;
    out.inner.b = node.inner.b + mul;
    out.tail = node.tail + (long long)(out.inner.a + out.inner.b);
    return out;
}

long long abi_outer_score(struct OuterNode node) {
    return (long long)(node.inner.a * 100) + (long long)(node.inner.b * 10) + node.tail;
}
