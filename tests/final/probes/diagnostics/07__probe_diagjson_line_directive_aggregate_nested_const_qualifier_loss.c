#line 8001 "virtual_typeconv_aggregate_nested_const_qualifier_loss.c"
struct Packet {
    int value;
};

int main(void) {
    struct Packet item = {0};
    const struct Packet *view = &item;
    const struct Packet * const *src = &view;
    struct Packet **dst;
    dst = src;
    return 0;
}
