#include <stdio.h>

#include "11__probe_wave88_indirect_no_proto_contract.h"

int main(void) {
    wave88_unprototyped_fn direct = wave88_promoted_callee;
    float direct_value = 1.25f;
    char direct_offset = 7;
    float factory_value = 2.50f;
    unsigned short factory_offset = 9;
    int direct_result = direct(direct_value, direct_offset);
    int factory_result =
        wave88_select_promoted_callee(1)(factory_value, factory_offset);

    printf("%d %d\n", direct_result, factory_result);
    return (direct_result == 132 && factory_result == 259) ? 0 : 1;
}
