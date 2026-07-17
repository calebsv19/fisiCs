#line 12920 "virtual_wave36_followup_error_order.c"
struct wave36_order_box {
    int value;
};

int main(void) {
    int first = wave36_order_first_missing;
    struct wave36_order_box box = { .value = 9 };
    int *ptr = box;
    return first + *ptr;
}
