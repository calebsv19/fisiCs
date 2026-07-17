#line 13120 "virtual_wave37_macro_followup_provenance.c"
#define W37_BAD_DECL(name) int name(int (*cb)(int, *), int value)

W37_BAD_DECL(wave37_macro_bad_decl);

int main(void) {
    int first = wave37_macro_first_missing;
    int *ptr = first;
    return first + *ptr;
}
