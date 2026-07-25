// SPDX-License-Identifier: Apache-2.0

#include <stdio.h>

#include "osp2_elf_admission_vectors.h"

int main(void) {
    if (osp2_elf_run_vectors() != 0) {
        puts(
            "OS-P2 case=osp2_elf_admission corpus="
            OSP2_ELF_CORPUS_ID " vectors=31 result=FAIL"
        );
        return 1;
    }
    puts(
        "OS-P2 case=osp2_elf_admission corpus="
        OSP2_ELF_CORPUS_ID " vectors=31 result=PASS"
    );
    return 0;
}
