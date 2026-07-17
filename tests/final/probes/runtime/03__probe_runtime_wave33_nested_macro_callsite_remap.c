#include <stdio.h>

#define W33_NEST_STR2(x) #x
#define W33_NEST_STR(x) W33_NEST_STR2(x)
#define W33_NEST_INNER(tag, line) printf("%s %s %d\n", W33_NEST_STR(tag), __FILE__, (line))
#define W33_NEST_MIDDLE(tag, line) W33_NEST_INNER(tag, line)
#define W33_NEST_OUTER(tag) W33_NEST_MIDDLE(tag, __LINE__)

#line 1820 "virtual_wave33_nested_callsite.c"
int main(void) {
    W33_NEST_OUTER(callsite_probe);
    return 0;
}
