// SPDX-License-Identifier: Apache-2.0

#include <stdio.h>

#include "osp2_queue_transition_vectors.h"

int main(void) {
    if (osp2_queue_run_vectors() != 0) {
        puts(
            "OS-P2 case=osp2_queue_transition corpus="
            OSP2_QUEUE_CORPUS_ID " vectors=44 result=FAIL"
        );
        return 1;
    }
    puts(
        "OS-P2 case=osp2_queue_transition corpus="
        OSP2_QUEUE_CORPUS_ID " vectors=44 result=PASS"
    );
    return 0;
}
