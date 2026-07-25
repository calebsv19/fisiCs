// SPDX-License-Identifier: Apache-2.0

#include <stdio.h>

#include "osp2_sync_rank_vectors.h"

int main(void) {
    if (osp2_sync_run_vectors() != 0) {
        puts(
            "OS-P2 case=osp2_sync_rank corpus="
            OSP2_SYNC_CORPUS_ID " vectors=51 result=FAIL"
        );
        return 1;
    }
    puts(
        "OS-P2 case=osp2_sync_rank corpus="
        OSP2_SYNC_CORPUS_ID " vectors=51 result=PASS"
    );
    return 0;
}
