// Minimal locale.h surface to ensure parser/PP handle macros and decls
#include <locale.h>

int main(void) {
    struct lconv *conv = localeconv();
    setlocale(LC_ALL, "");
    return conv != 0;
}
