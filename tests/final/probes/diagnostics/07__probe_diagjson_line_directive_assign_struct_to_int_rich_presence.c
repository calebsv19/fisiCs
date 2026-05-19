#line 5301 "virtual_types_assign_struct_to_int_diagjson_probe.c"
struct Payload {
    int value;
};

int main(void) {
    struct Payload payload = {1};
    int number = payload;
    return number;
}
