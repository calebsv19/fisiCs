#line 8601 "virtual_typeconv_lexical_tag_pointer_comparison.c"
struct S {
    int outer_value;
};

int main(void) {
    struct S outer = {1};
    struct S *captured = &outer;
    {
        struct T {
            long inner_value;
            long extra;
        } inner = {2, 3};
        int equal = captured == &inner;
        int ordered = captured < &inner;
        return equal + ordered;
    }
}
