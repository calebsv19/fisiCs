// SPDX-License-Identifier: Apache-2.0

#include <stdio.h>

#include "osp2_scheduler_transition_vectors.h"

int main(void) {
    if (osp2_scheduler_run_vectors() != 0) {
        puts(
            "OS-P2 case=osp2_scheduler_transition corpus="
            OSP2_SCHED_CORPUS_ID " vectors=60 result=FAIL"
        );
        return 1;
    }
    puts(
        "OS-P2 case=osp2_scheduler_transition corpus="
        OSP2_SCHED_CORPUS_ID " vectors=60 result=PASS"
    );
    return 0;
}
