#line 4101 "virtual_init_nested_designator_array_oob_probe.c"
int grid[2][2] = {
    [1] = {
        [3] = 1
    }
};

int main(void) {
    return grid[0][0];
}
