#line 19301 "virtual_wave83_prototype_first_double_control.c"
int wave83_double_control(double value);
int wave83_double_control();

int wave83_double_control(double value) {
    return value > 0.0;
}

int main(void) {
    return wave83_double_control(3.5) == 1 ? 0 : 1;
}
