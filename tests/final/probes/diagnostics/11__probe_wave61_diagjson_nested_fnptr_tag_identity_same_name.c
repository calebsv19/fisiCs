#line 17601 "virtual_wave61_nested_fnptr_tag_identity_same_name.c"
struct CallbackPayload {
    int outer_value;
};

typedef int (*OuterCallback)(struct CallbackPayload *value);

int main(void) {
    OuterCallback outer = 0;
    {
        struct CallbackPayload {
            long inner_value;
            long extra;
        };
        typedef int (*InnerCallback)(struct CallbackPayload *value);
        InnerCallback inner = 0;
        outer = inner;
    }
    return outer != 0;
}
