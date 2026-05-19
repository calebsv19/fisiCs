#line 4701 "virtual_types_conv_pointer_init_struct_diagjson_probe.c"
struct Payload {
    int value;
};

int main(void) {
    struct Payload payload = {7};
    int *p = payload;
    return p != 0;
}
