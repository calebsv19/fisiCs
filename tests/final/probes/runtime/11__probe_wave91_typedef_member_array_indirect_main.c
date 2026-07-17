#include <stdio.h>

#include "11__probe_wave91_typedef_member_array_indirect_contract.h"

int main(void) {
    Wave91Dispatch dispatch = wave91_make_dispatch(0);
    Wave91Samples samples = {2, 3, 5};
    Wave91Payload seed = {{10, 20, 30}, 7};
    Wave91Payload first = dispatch.transforms[0](samples, seed);

    samples[1] = 4;
    Wave91Payload second = dispatch.transforms[1](samples, seed);

    printf("%lld %lld %lld %d %lld %lld %lld %d %d\n",
           first.lane[0],
           first.lane[1],
           first.lane[2],
           first.stamp,
           second.lane[0],
           second.lane[1],
           second.lane[2],
           second.stamp,
           dispatch.generation);
    return 0;
}
