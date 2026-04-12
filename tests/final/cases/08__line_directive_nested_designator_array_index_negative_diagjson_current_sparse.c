#line 6601 "virtual_init_nested_designator_array_negative_case.c"
int guard_nested_negative = 0;
int grid_nested_negative[2][2] = {[1] = {[-1] = 1}};

int main(void) {
    return grid_nested_negative[1][1] + guard_nested_negative;
}
