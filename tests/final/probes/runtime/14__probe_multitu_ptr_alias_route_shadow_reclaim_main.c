#include <stdio.h>

extern unsigned ptr_alias_route_shadow_reclaim(unsigned seed);

int main(void) {
    printf("%u %u\n",
           ptr_alias_route_shadow_reclaim(87u),
           ptr_alias_route_shadow_reclaim(171u));
    return 0;
}
