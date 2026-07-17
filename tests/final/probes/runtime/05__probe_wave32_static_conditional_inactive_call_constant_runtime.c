#include <stdio.h>

static int wave32_call_count;
static int wave32_probe_value(void);

static int wave32_static_value = 1 ? 7 : wave32_probe_value();

static int wave32_probe_value(void) {
    ++wave32_call_count;
    return 99;
}

int main(void) {
    printf("%d %d\n", wave32_static_value, wave32_call_count);
    return 0;
}
