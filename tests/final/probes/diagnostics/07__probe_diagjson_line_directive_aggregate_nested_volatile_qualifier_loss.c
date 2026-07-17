#line 8101 "virtual_typeconv_aggregate_nested_volatile_qualifier_loss.c"
struct Packet {
    int value;
};

int main(void) {
    struct Packet item = {0};
    volatile struct Packet *view = &item;
    volatile struct Packet * volatile *src = &view;
    struct Packet **dst;
    dst = src;
    return 0;
}
