#line 10701 "virtual_scope_block_conflicting_types_diag_text_strict.c"
int main(void) {
    int a = 1;
    {
        int a = 2;
        float a = 3.0f;
        return (int)a;
    }
}
