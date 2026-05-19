#line 6301 "virtual_types_nested_designator_array_negative_probe.c"
int probe_wave48_nested_negative_guard = 0;
int probe_wave48_nested_negative_grid[2][2] = {[1] = {[-1] = 1}};

int main(void) {
    return probe_wave48_nested_negative_grid[1][1] + probe_wave48_nested_negative_guard;
}
