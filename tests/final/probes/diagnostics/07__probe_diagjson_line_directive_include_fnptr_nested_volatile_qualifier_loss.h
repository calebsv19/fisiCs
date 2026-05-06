#line 3201 "virtual_typeconv_include_fnptr_nested_volatile_qualifier_loss.h"
static int fnptr_nested_volatile_add1(int x) {
    return x + 1;
}

static int fnptr_nested_volatile_qualifier_loss_case(void) {
    int (*fp)(int) = fnptr_nested_volatile_add1;
    int (* volatile *src)(int) = &fp;
    int (**dst)(int);
    dst = src;
    return 0;
}
