#line 19001 "virtual_wave27_nested_factory_equality_same_name.c"
struct Payload {
    int outer;
};

typedef struct Payload (*OuterCallback)(struct Payload);
typedef OuterCallback (*OuterFactory)(OuterCallback);

int main(void) {
    OuterFactory lhs = 0;
    {
        struct Payload {
            long inner;
            long extra;
        };
        typedef struct Payload (*InnerCallback)(struct Payload);
        typedef InnerCallback (*InnerFactory)(InnerCallback);
        InnerFactory rhs = 0;
        return lhs == rhs;
    }
}
