#line 17801 "virtual_wave63_nested_fnptr_conditional_alpha_name.c"
struct CallbackPayload {
    int outer_value;
};

typedef int (*OuterCallback)(struct CallbackPayload *value);

int wave63_choose(int choose) {
    OuterCallback outer = 0;
    {
        struct InnerPayload {
            long inner_value;
            long extra;
        };
        typedef int (*InnerCallback)(struct InnerPayload *value);
        InnerCallback inner = 0;
        (void)(choose ? outer : inner);
        return 0;
    }
}


int main(void) {
    return wave63_choose(1);
}
