#line 13240 "virtual_wave38_macro_while.c"
#define W38_BAD_WHILE(value) while ((value) +) { (value)++; }

int main(void) {
    int total = 0;
    W38_BAD_WHILE(total)
    int later = wave38_macro_while_missing;
    int *ptr = later;
    return total + later + *ptr;
}
