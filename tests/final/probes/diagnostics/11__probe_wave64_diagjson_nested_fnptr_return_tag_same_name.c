#line 17901 "virtual_wave64_nested_fnptr_return_tag_same_name.c"
struct FactoryPayload {
    int outer_value;
};

typedef struct FactoryPayload *(*OuterFactory)(void);

int main(void) {
    OuterFactory outer = 0;
    {
        struct FactoryPayload {
            long inner_value;
            long extra;
        };
        typedef struct FactoryPayload *(*InnerFactory)(void);
        InnerFactory inner = 0;
        outer = inner;
    }
    return outer != 0;
}
