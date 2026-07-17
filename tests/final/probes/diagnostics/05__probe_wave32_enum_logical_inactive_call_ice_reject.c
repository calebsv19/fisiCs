int wave32_probe_value(void);

#line 32002 "wave32_enum_logical_ice.c"
enum { WAVE32_ENUM_LOGICAL = 1 || wave32_probe_value() };

int main(void) {
    return WAVE32_ENUM_LOGICAL;
}
