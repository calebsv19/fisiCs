#line 8301 "virtual_typeconv_include_aggregate_nested_volatile_qualifier_loss.h"
struct IncludePacketVolatile {
    int value;
};

static int aggregate_nested_volatile_qualifier_loss_case(void) {
    struct IncludePacketVolatile item = {0};
    volatile struct IncludePacketVolatile *view = &item;
    volatile struct IncludePacketVolatile * volatile *src = &view;
    struct IncludePacketVolatile **dst;
    dst = src;
    return 0;
}
