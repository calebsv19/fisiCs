#define PRAGMA_INNER(tokens) _Pragma(#tokens)
#define PRAGMA_OUTER(tokens) PRAGMA_INNER(tokens)
#define PRAGMA_PAYLOAD STDC FP_CONTRACT OFF

PRAGMA_OUTER(PRAGMA_PAYLOAD) static int pragma_macro_value(void) {
    return 78;
}

int main(void) {
    return pragma_macro_value() == 78 ? 0 : 1;
}
