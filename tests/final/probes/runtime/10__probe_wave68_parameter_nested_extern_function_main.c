#include <stdio.h>

static int wave68_call_external_route(int wave68_route) {
    {
        extern int wave68_route(void);
        return wave68_route();
    }
}

int main(void) {
    printf("%d %d\n",
           wave68_call_external_route(3),
           wave68_call_external_route(99));
    return 0;
}
