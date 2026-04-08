#line 700 "virtual_include_macro_diagjson_header.h"
#define BAD_EXPR_HDR(x) ((x) + )

static int trigger_macro_diag(void) {
    int x = 1;
    int y = BAD_EXPR_HDR(x);
    return y;
}

