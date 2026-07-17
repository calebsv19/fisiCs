#include <stdio.h>

#define W33_CAT2(a, b) a##b
#define W33_CAT(a, b) W33_CAT2(a, b)
#define W33_STR2(x) #x
#define W33_STR(x) W33_STR2(x)
#define W33_DECL(name, value) enum { W33_CAT(w33_value_, name) = value }
#define W33_REPORT(name) \
    printf("%d %s %s %d\n", W33_CAT(w33_value_, name), W33_STR(W33_CAT(w33_value_, name)), __FILE__, __LINE__)

W33_DECL(alpha, 73);

#line 1700 "virtual_wave33_token_paste.c"
int main(void) {
    W33_REPORT(alpha);
    return 0;
}
