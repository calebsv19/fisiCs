#line 19201 "virtual_wave82_no_prototype_double_control.c"
int wave82_double_control();
int wave82_double_control(double value);

int wave82_double_control(double value) {
    return value > 0.0;
}

int main(void) {
    return wave82_double_control(2.5) == 1 ? 0 : 1;
}
