#line 13520 "virtual_wave39_enum_initializer_recovery.c"
enum Wave39Kind {
    WAVE39_FIRST = ,
    WAVE39_SECOND
};

int main(void) {
    int later = wave39_enum_initializer_missing;
    int *ptr = later;
    return WAVE39_SECOND + later + *ptr;
}
