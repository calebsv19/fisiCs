#include <stdio.h>

#define W36_PROV_CAT2(a, b) a##b
#define W36_PROV_CAT(a, b) W36_PROV_CAT2(a, b)
#define W36_PROV_STR2(x) #x
#define W36_PROV_STR(x) W36_PROV_STR2(x)
#define W36_PROV_DECL(value) enum { W36_PROV_CAT(w36_prov_line_, __LINE__) = (value) }

#line 4200 "virtual_wave36_paste_header.h"
W36_PROV_DECL(44);

#line 4300 "virtual_wave36_paste_main.c"
int main(void) {
    printf("%d %s %s %d\n", w36_prov_line_4200, W36_PROV_STR(__LINE__), __FILE__, __LINE__);
    return 0;
}
