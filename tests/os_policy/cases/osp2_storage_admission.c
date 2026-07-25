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
) {
    if (magic != OSP2_STORAGE_MAGIC || version != 1) {
        return 0;
    }
    if (block_size != 4096 || total_blocks < 8) {
        return 0;
    }
    if (journal_start < 2 || journal_start >= total_blocks) {
        return 0;
    }
    if (journal_blocks == 0 || journal_blocks > total_blocks - journal_start) {
        return 0;
    }
    return 1;
}

osp2_u64 osp2_extent_admit(
    osp2_u64 start,
    osp2_u64 count,
    osp2_u64 total_blocks,
    osp2_u64 reserved_end
) {
    if (count == 0 || start < reserved_end || start >= total_blocks) {
        return 0;
    }
    return count <= total_blocks - start;
}

osp2_u64 osp2_result_transition_admit(
    osp2_u64 current_state,
    osp2_u64 next_state,
    osp2_u64 durable_commit
) {
    if (current_state == 0 && next_state == 1) {
        return 1;
    }
    if (current_state == 1 && next_state == 2) {
        return durable_commit != 0;
    }
    if (current_state == 2 && next_state == 0) {
        return 1;
    }
    return 0;
}
