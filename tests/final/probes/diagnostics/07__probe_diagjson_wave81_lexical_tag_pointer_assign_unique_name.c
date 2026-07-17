#line 8401 "virtual_typeconv_lexical_tag_pointer_unique_name.c"
struct S {
    int outer_value;
};

int main(void) {
    struct S outer = {1};
    struct S *outer_ptr = &outer;
    {
        struct InnerS {
            long inner_value;
            long extra;
        };
        struct InnerS inner = {2, 3};
        struct InnerS *inner_ptr = &inner;
        outer_ptr = inner_ptr;
    }
    return outer_ptr->outer_value;
}
