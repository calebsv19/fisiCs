#line 13020 "virtual_wave37_direct_param_hole.c"
int wave37_direct_bad_param(int (*cb)(int, *), int value);

int main(void) {
    int later = wave37_direct_param_missing;
    int *ptr = later;
    return later + *ptr;
}
