// SPDX-License-Identifier: Apache-2.0

#include <stdio.h>
#include "osp2_process_lifecycle_vectors.h"

int main(void) {
    if (osp2_process_run_vectors() != 0) {
        puts("OS-P2 case=osp2_process_lifecycle corpus="
             OSP2_PROCESS_CORPUS_ID " vectors=34 result=FAIL");
        return 1;
    }
    puts("OS-P2 case=osp2_process_lifecycle corpus="
         OSP2_PROCESS_CORPUS_ID " vectors=34 result=PASS");
    return 0;
}
