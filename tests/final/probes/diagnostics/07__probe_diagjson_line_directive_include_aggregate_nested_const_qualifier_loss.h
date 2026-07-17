#line 8201 "virtual_typeconv_include_aggregate_nested_const_qualifier_loss.h"
struct IncludePacketConst {
    int value;
};

static int aggregate_nested_const_qualifier_loss_case(void) {
    struct IncludePacketConst item = {0};
    const struct IncludePacketConst *view = &item;
    const struct IncludePacketConst * const *src = &view;
    struct IncludePacketConst **dst;
    dst = src;
    return 0;
}
