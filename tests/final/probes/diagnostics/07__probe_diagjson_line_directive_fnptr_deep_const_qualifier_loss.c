#line 2901 "virtual_typeconv_fnptr_deep_const_qualifier_loss.c"
int add1(int x) {
    return x + 1;
}

int main(void) {
    int (*fp)(int) = add1;
    int (**pp)(int) = &fp;
    int (** const *src)(int) = &pp;
    int (***dst)(int);
    dst = src;
    return 0;
}
