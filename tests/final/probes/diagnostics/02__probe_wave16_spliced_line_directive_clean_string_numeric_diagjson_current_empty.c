#\
line 5040 "virtual_lexer_wave16_clean_boundary.c"
int wave16_clean_boundary(void) {
    const char *s = "left"\
"right\x21";
    unsigned long value = 0x1\
2UL + 3\
4U;
    return s[0] + s[5] + (int)value;
}
