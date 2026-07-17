int wave32_probe_value(void);

#line 32001 "wave32_enum_conditional_ice.c"
enum { WAVE32_ENUM_CONDITIONAL = 1 ? 7 : wave32_probe_value() };

int main(void) {
    return WAVE32_ENUM_CONDITIONAL;
}
