// SPDX-License-Identifier: Apache-2.0

#include "osp2_sync_rank_vectors.h"

void osp_guest_serial_write(const char* text);
void osp_guest_exit(unsigned int value);

void osp_guest_main(void) {
    if (osp2_sync_run_vectors() != 0) {
        osp_guest_serial_write(
            "OS-P2 guest case=osp2_sync_rank corpus="
            OSP2_SYNC_CORPUS_ID " vectors=51 result=FAIL\n"
        );
        osp_guest_exit(0x3F);
    }
    osp_guest_serial_write(
        "OS-P2 guest case=osp2_sync_rank corpus="
        OSP2_SYNC_CORPUS_ID " vectors=51 result=PASS\n"
    );
    osp_guest_exit(0x2A);
}
