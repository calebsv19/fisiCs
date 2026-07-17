#line 17501 "virtual_wave60_block_function_redecl_same_name.c"
struct OuterPayload {
    int outer_value;
};

struct OuterPayload *wave60_make(void);

int main(void) {
    {
        struct OuterPayload {
            long inner_value;
            long extra;
        };
        struct OuterPayload *wave60_make(void);
    }
    return 0;
}
