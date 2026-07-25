// SPDX-License-Identifier: Apache-2.0

#include <stdio.h>
#include "osp2_kernel_object_vectors.h"

int main(void) {
    if (osp2_kobj_run_vectors() != 0) {
        puts("OS-P2 case=osp2_kernel_object corpus=" OSP2_KOBJ_CORPUS_ID
             " vectors=51 result=FAIL");
        return 1;
    }
    puts("OS-P2 case=osp2_kernel_object corpus=" OSP2_KOBJ_CORPUS_ID
         " vectors=51 result=PASS");
    return 0;
}
