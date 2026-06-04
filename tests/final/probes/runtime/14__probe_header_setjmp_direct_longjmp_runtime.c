#include <setjmp.h>
#include <stdio.h>

static jmp_buf g_probe_header_setjmp_direct_env;
static volatile int g_probe_header_setjmp_direct_seen = 0;

int main(void) {
    int checkpoint = setjmp(g_probe_header_setjmp_direct_env);

    if (checkpoint == 0) {
        g_probe_header_setjmp_direct_seen = 1;
        longjmp(g_probe_header_setjmp_direct_env, 7);
    }

    printf(
        "direct=%d seen=%d\n",
        checkpoint,
        (int)g_probe_header_setjmp_direct_seen);
    return 0;
}
