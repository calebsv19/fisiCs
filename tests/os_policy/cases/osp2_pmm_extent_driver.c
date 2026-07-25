// SPDX-License-Identifier: Apache-2.0

#include <stdio.h>

#include "osp2_pmm_extent_vectors.h"

int main(void) {
    if (osp2_extent_run_vectors() != 0) {
        puts(
            "OS-P2 case=osp2_pmm_extent corpus="
            OSP2_EXTENT_CORPUS_ID " vectors=62 result=FAIL"
        );
        return 1;
    }
    puts(
        "OS-P2 case=osp2_pmm_extent corpus="
        OSP2_EXTENT_CORPUS_ID " vectors=62 result=PASS"
    );
    return 0;
}
