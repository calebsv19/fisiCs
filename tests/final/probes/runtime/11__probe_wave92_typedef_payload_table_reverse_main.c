#include <stdio.h>

#include "11__probe_wave92_typedef_payload_table_reverse_contract.h"

int main(void) {
    const Wave92TransformTable *table = wave92_get_table(0);
    Wave92Payload seed = {{10, 20, 30}, 7};
    Wave92Lane delta = {2, 3, 5};
    Wave92Payload first = (*table)[0](seed, delta, 3);

    delta[1] = 4;
    Wave92Payload second = (*table)[1](seed, delta, 3);

    printf("%lld %lld %lld %d | %lld %lld %lld %d\n",
           first.lane[0],
           first.lane[1],
           first.lane[2],
           first.stamp,
           second.lane[0],
           second.lane[1],
           second.lane[2],
           second.stamp);
    return 0;
}
