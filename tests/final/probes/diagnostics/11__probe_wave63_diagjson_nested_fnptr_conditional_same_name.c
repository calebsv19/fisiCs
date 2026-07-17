#line 17801 "virtual_wave63_nested_fnptr_conditional_same_name.c"
struct CallbackPayload {
    int outer_value;
};

typedef int (*OuterCallback)(struct CallbackPayload *value);

int wave63_choose(int choose) {
    OuterCallback outer = 0;
    {
        struct CallbackPayload {
            long inner_value;
            long extra;
        };
        typedef int (*InnerCallback)(struct CallbackPayload *value);
        InnerCallback inner = 0;
        (void)(choose ? outer : inner);
        return 0;
    }
}

int main(void) {
    return wave63_choose(1);
}
