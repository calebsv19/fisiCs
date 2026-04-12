#line 10801 "virtual_scope_include_block_conflicting_types_diagjson_strict.h"
int f(void) {
    int a = 1;
    {
        int a = 2;
        float a = 3.0f;
        return (int)a;
    }
}
