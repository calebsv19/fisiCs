#line 6501 "virtual_types_nested_designator_array_nonconst_probe.c"
int probe_wave50_nested_nonconst_index = 1;
int probe_wave50_nested_nonconst_grid[2][2] = {[1] = {[probe_wave50_nested_nonconst_index] = 1}};

int main(void) {
    return probe_wave50_nested_nonconst_grid[1][1];
}
