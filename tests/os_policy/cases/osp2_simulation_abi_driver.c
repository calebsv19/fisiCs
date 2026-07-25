// SPDX-License-Identifier: Apache-2.0

#include <stdio.h>
#include "osp2_simulation_abi_vectors.h"

int main(void) {
    if (osp2_sim_run_vectors() != 0) {
        puts("OS-P2 case=osp2_simulation_abi corpus=" OSP2_SIM_CORPUS_ID
             " vectors=38 result=FAIL");
        return 1;
    }
    puts("OS-P2 case=osp2_simulation_abi corpus=" OSP2_SIM_CORPUS_ID
         " vectors=38 result=PASS");
    return 0;
}
