int axis1_wave6_alias_slot(void) {
    return 41;
}

extern int axis1_wave6_alias_read(void);

int main(void) {
    return axis1_wave6_alias_slot() + axis1_wave6_alias_read();
}
