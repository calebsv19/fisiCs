#line 8501 "virtual_typeconv_lexical_tag_pointer_conditional.c"
struct S {
    int outer_value;
};

int main(void) {
    struct S outer = {1};
    struct S *captured = &outer;
    int choose = outer.outer_value;
    {
        struct S {
            long inner_value;
            long extra;
        } inner = {2, 3};
        void *selected = choose ? captured : &inner;
        return selected != 0;
    }
}
