#line 2901 "virtual_init_designator_array_oob_case.c"
int arr[2] = {[3] = 1};

int main(void) {
    return arr[0];
}
