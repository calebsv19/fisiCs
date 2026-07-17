#line 8401 "virtual_typeconv_lexical_tag_pointer_same_name.c"
struct S {
    int outer_value;
};

int main(void) {
    struct S outer = {1};
    struct S *outer_ptr = &outer;
    {
        struct S {
            long inner_value;
            long extra;
        };
        struct S inner = {2, 3};
        struct S *inner_ptr = &inner;
        outer_ptr = inner_ptr;
    }
    return outer_ptr->outer_value;
}
