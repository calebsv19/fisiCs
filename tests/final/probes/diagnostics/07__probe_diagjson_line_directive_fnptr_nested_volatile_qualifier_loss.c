#line 2801 "virtual_typeconv_fnptr_nested_volatile_qualifier_loss.c"
int add1(int x) {
    return x + 1;
}

int main(void) {
    int (*fp)(int) = add1;
    int (* volatile *src)(int) = &fp;
    int (**dst)(int);
    dst = src;
    return 0;
}
