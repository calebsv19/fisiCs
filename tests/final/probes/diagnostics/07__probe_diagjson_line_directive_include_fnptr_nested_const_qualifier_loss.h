#line 3101 "virtual_typeconv_include_fnptr_nested_const_qualifier_loss.h"
static int fnptr_nested_const_add1(int x) {
    return x + 1;
}

static int fnptr_nested_const_qualifier_loss_case(void) {
    int (*fp)(int) = fnptr_nested_const_add1;
    int (* const *src)(int) = &fp;
    int (**dst)(int);
    dst = src;
    return 0;
}
