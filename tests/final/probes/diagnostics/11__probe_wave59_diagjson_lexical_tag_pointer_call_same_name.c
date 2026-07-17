#line 17401 "virtual_wave59_lexical_tag_pointer_call_same_name.c"
struct S {
    int outer_value;
};

int wave59_accept(struct S *value);

int main(void) {
    {
        struct S {
            long inner_value;
            long extra;
        };
        struct S inner = {2, 3};
        return wave59_accept(&inner);
    }
}
