#include <stdio.h>

#define W35_STACK_SLOT north
#define W35_STACK_VALUE 5
#include "03__probe_runtime_wave35_line_stack_reentry_value.h"

#undef W35_STACK_SLOT
#undef W35_STACK_VALUE
#define W35_STACK_SLOT south
#define W35_STACK_VALUE 7
#include "03__probe_runtime_wave35_line_stack_reentry_value.h"

#line 3800 "virtual_wave35_stack_main.c"
int main(void) {
    printf("%d %d %s %d\n", w35_stack_north, w35_stack_south, __FILE__, __LINE__);
    return 0;
}
