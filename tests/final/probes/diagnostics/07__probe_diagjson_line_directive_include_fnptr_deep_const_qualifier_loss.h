#line 3301 "virtual_typeconv_include_fnptr_deep_const_qualifier_loss.h"
static int fnptr_deep_const_add1(int x) {
    return x + 1;
}

static int fnptr_deep_const_qualifier_loss_case(void) {
    int (*fp)(int) = fnptr_deep_const_add1;
    int (**pp)(int) = &fp;
    int (** const *src)(int) = &pp;
    int (***dst)(int);
    dst = src;
    return 0;
}
