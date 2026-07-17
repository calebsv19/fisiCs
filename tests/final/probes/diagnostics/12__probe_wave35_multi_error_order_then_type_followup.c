#line 12700 "virtual_wave35_multi_error_order_boundary.c"
struct wave35_box {
    int value;
};

int main(void) {
    int first = wave35_multi_error_first_missing;
    struct wave35_box box = { .value = 4 };
    int *ptr = &box;
    return first + *ptr;
}
