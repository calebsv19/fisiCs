#line 6601 "virtual_init_nested_designator_array_negative_probe.c"
int guard_nested_negative_probe = 0;
int grid_nested_negative_probe[2][2] = {[1] = {[-1] = 1}};

int main(void) {
    return grid_nested_negative_probe[1][1] + guard_nested_negative_probe;
}
