// SPDX-License-Identifier: Apache-2.0

#include <stdio.h>

#include "osp2_job_admission_vectors.h"

int main(void) {
    if (osp2_job_run_vectors() != 0) {
        puts(
            "OS-P2 case=osp2_job_admission corpus="
            OSP2_JOB_CORPUS_ID " vectors=27 result=FAIL"
        );
        return 1;
    }
    puts(
        "OS-P2 case=osp2_job_admission corpus="
        OSP2_JOB_CORPUS_ID " vectors=27 result=PASS"
    );
    return 0;
}
