#line 10601 "virtual_scope_block_conflicting_types_diagjson_strict.c"
int main(void) {
    int a = 1;
    {
        int a = 2;
        float a = 3.0f;
        return (int)a;
    }
}
