#include <setjmp.h>
#include <stdio.h>

static jmp_buf g_probe_header_setjmp_helper_env;
static volatile int g_probe_header_setjmp_helper_seen = 0;

static void probe_header_setjmp_helper_jump(int value) {
    g_probe_header_setjmp_helper_seen = value + 1;
    longjmp(g_probe_header_setjmp_helper_env, value);
}

int main(void) {
    int checkpoint = setjmp(g_probe_header_setjmp_helper_env);

    if (checkpoint == 0) {
        probe_header_setjmp_helper_jump(9);
    }

    printf(
        "helper=%d seen=%d\n",
        checkpoint,
        (int)g_probe_header_setjmp_helper_seen);
    return 0;
}
