#line 17701 "virtual_wave62_nested_fnptr_initializer_same_name.c"
struct CallbackPayload {
    int outer_value;
};

typedef int (*OuterCallback)(struct CallbackPayload *value);

int main(void) {
    {
        struct CallbackPayload {
            long inner_value;
            long extra;
        };
        typedef int (*InnerCallback)(struct CallbackPayload *value);
        InnerCallback inner = 0;
        OuterCallback outer = inner;
        return outer != 0;
    }
}
