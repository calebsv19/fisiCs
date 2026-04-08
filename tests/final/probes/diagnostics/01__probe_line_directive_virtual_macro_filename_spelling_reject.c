#define VFILE "virtual_macro_phase01_probe.c"
#line 512 VFILE
#define BAD_EXPR(x) ((x) + )

int main(void) {
    int x = 1;
    int y = BAD_EXPR(x);
    return y;
}
