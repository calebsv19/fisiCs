#line 19101 "virtual_wave27_nested_factory_inequality_alpha_name.c"
struct OuterPayload {
    int outer;
};

typedef struct OuterPayload (*OuterCallback)(struct OuterPayload);
typedef OuterCallback (*OuterFactory)(OuterCallback);

int main(void) {
    OuterFactory lhs = 0;
    {
        struct InnerPayload {
            long inner;
            long extra;
        };
        typedef struct InnerPayload (*InnerCallback)(struct InnerPayload);
        typedef InnerCallback (*InnerFactory)(InnerCallback);
        InnerFactory rhs = 0;
        return lhs != rhs;
    }
}
