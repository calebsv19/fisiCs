#line 4100 "virtual_wave41_variadic_paste.c"
#define W41_CAT_RAW(left, right) left ## right
#define W41_CAT(left, right) W41_CAT_RAW(left, right)
#define W41_VARIADIC_PASTE(prefix, ...) W41_CAT(prefix, __VA_ARGS__)

int main(void) {
    int existing_name = 1;
    (void)W41_VARIADIC_PASTE(missing_, name);
    return existing_name;
}
