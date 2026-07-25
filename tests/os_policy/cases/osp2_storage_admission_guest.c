// SPDX-License-Identifier: Apache-2.0

typedef unsigned long osp2_u64;

#define OSP2_STORAGE_MAGIC 0x435753494D465331UL

osp2_u64 osp2_storage_header_admit(
    osp2_u64 magic,
    osp2_u64 version,
    osp2_u64 block_size,
    osp2_u64 total_blocks,
    osp2_u64 journal_start,
    osp2_u64 journal_blocks
);
osp2_u64 osp2_extent_admit(
    osp2_u64 start,
    osp2_u64 count,
    osp2_u64 total_blocks,
    osp2_u64 reserved_end
);
osp2_u64 osp2_result_transition_admit(
    osp2_u64 current_state,
    osp2_u64 next_state,
    osp2_u64 durable_commit
);
void osp_guest_serial_write(const char* text);
void osp_guest_exit(unsigned int value);

static osp2_u64 expect_u64(osp2_u64 actual, osp2_u64 expected) {
    return actual == expected ? 0 : 1;
}

void osp_guest_main(void) {
    osp2_u64 failures = 0;

    failures += expect_u64(
        osp2_storage_header_admit(OSP2_STORAGE_MAGIC, 1, 4096, 64, 48, 16), 1
    );
    failures += expect_u64(
        osp2_storage_header_admit(0, 1, 4096, 64, 48, 16), 0
    );
    failures += expect_u64(
        osp2_storage_header_admit(OSP2_STORAGE_MAGIC, 2, 4096, 64, 48, 16), 0
    );
    failures += expect_u64(
        osp2_storage_header_admit(OSP2_STORAGE_MAGIC, 1, 512, 64, 48, 16), 0
    );
    failures += expect_u64(
        osp2_storage_header_admit(OSP2_STORAGE_MAGIC, 1, 4096, 7, 2, 1), 0
    );
    failures += expect_u64(
        osp2_storage_header_admit(OSP2_STORAGE_MAGIC, 1, 4096, 64, 1, 1), 0
    );
    failures += expect_u64(
        osp2_storage_header_admit(OSP2_STORAGE_MAGIC, 1, 4096, 64, 64, 1), 0
    );
    failures += expect_u64(
        osp2_storage_header_admit(OSP2_STORAGE_MAGIC, 1, 4096, 64, 48, 17), 0
    );
    failures += expect_u64(osp2_extent_admit(8, 1, 64, 8), 1);
    failures += expect_u64(osp2_extent_admit(63, 1, 64, 8), 1);
    failures += expect_u64(osp2_extent_admit(7, 1, 64, 8), 0);
    failures += expect_u64(osp2_extent_admit(8, 0, 64, 8), 0);
    failures += expect_u64(osp2_extent_admit(64, 1, 64, 8), 0);
    failures += expect_u64(osp2_extent_admit(63, 2, 64, 8), 0);
    failures += expect_u64(osp2_result_transition_admit(0, 1, 0), 1);
    failures += expect_u64(osp2_result_transition_admit(1, 2, 0), 0);
    failures += expect_u64(osp2_result_transition_admit(1, 2, 1), 1);
    failures += expect_u64(osp2_result_transition_admit(2, 0, 0), 1);

    if (failures != 0) {
        osp_guest_serial_write(
            "OS-P2 guest case=osp2_storage_admission vectors=18 result=FAIL\n"
        );
        osp_guest_exit(0x3F);
    }
    osp_guest_serial_write(
        "OS-P2 guest case=osp2_storage_admission vectors=18 result=PASS\n"
    );
    osp_guest_exit(0x2A);
}
