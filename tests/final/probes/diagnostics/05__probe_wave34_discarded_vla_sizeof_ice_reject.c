#line 34101 "wave34_discarded_vla_sizeof_ice.c"
int wave34_vla_select(int value, int bound) {
    switch (value) {
        case 1 ? 7 : sizeof(int[bound]):
            return 7;
        case 1 || sizeof(int[bound]):
            return 1;
        default:
            return 0;
    }
}

int main(void) {
    return wave34_vla_select(7, 4);
}
