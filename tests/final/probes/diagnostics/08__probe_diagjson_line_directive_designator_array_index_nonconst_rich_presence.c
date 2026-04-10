#line 4401 "virtual_init_designator_array_nonconst_probe.c"
int idx_probe_nonconst = 1;
int arr_probe_nonconst[2] = {[idx_probe_nonconst] = 1};

int main(void) {
    return arr_probe_nonconst[0];
}
