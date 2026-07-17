#line 13400 "virtual_wave39_nested_else_recovery.c"
int main(void) {
    int gate = 1;
    if (gate) {
        if (gate +) {
            gate = 2;
        } else {
            gate = 3;
        }
    } else {
        gate = 4;
    }
    int later = wave39_nested_else_missing;
    int *ptr = later;
    return gate + later + *ptr;
}
