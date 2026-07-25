// SPDX-License-Identifier: Apache-2.0

typedef unsigned char osp0_u8;
typedef unsigned long osp0_u64;

osp0_u64 osp0_read_le64(const osp0_u8* bytes) {
    return (osp0_u64)bytes[0]
        | ((osp0_u64)bytes[1] << 8)
        | ((osp0_u64)bytes[2] << 16)
        | ((osp0_u64)bytes[3] << 24)
        | ((osp0_u64)bytes[4] << 32)
        | ((osp0_u64)bytes[5] << 40)
        | ((osp0_u64)bytes[6] << 48)
        | ((osp0_u64)bytes[7] << 56);
}
osp0_u64 osp0_checked_span(osp0_u64 offset, osp0_u64 length, osp0_u64 capacity) {
    if (offset > capacity) {
        return 0;
    }
    return length <= capacity - offset;
}

osp0_u64 osp0_lock_order(osp0_u64 held_rank, osp0_u64 requested_rank) {
    if (requested_rank < 1 || requested_rank > 3) {
        return 0;
    }
    if (held_rank == 0) {
        return 1;
    }
    return held_rank < requested_rank;
}

osp0_u64 osp0_generation_accept(
    osp0_u64 allocated,
    osp0_u64 slot_generation,
    osp0_u64 token_generation
) {
    return allocated != 0
        && token_generation != 0
        && slot_generation == token_generation;
}
