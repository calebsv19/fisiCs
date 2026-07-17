#line 17701 "virtual_wave62_nested_fnptr_initializer_alpha_name.c"
struct CallbackPayload {
    int outer_value;
};

typedef int (*OuterCallback)(struct CallbackPayload *value);

int main(void) {
    {
        struct InnerPayload {
            long inner_value;
            long extra;
        };
        typedef int (*InnerCallback)(struct InnerPayload *value);
        InnerCallback inner = 0;
        OuterCallback outer = inner;
        return outer != 0;
    }
}
