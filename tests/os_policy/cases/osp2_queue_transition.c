// SPDX-License-Identifier: Apache-2.0

typedef unsigned char osp2_u8;
typedef unsigned int osp2_u32;
typedef unsigned long osp2_u64;

#define OSP2_QUEUE_SECTOR 512UL
#define OSP2_QUEUE_STATE_EMPTY 0UL
#define OSP2_QUEUE_STATE_PENDING 1UL
#define OSP2_QUEUE_STATE_RUNNING 2UL
#define OSP2_QUEUE_STATE_COMPLETE 3UL
#define OSP2_QUEUE_STATE_FAILED 4UL
#define OSP2_QUEUE_STATE_CANCELLED 5UL
#define OSP2_QUEUE_FLAG_CANCEL_PENDING 1UL
#define OSP2_QUEUE_OK 0UL
#define OSP2_QUEUE_FORMAT 1UL
#define OSP2_QUEUE_RESOURCE 2UL
#define OSP2_QUEUE_CANCEL 3UL
#define OSP2_QUEUE_COMPUTE_FAILED 4UL
#define OSP2_QUEUE_INTERRUPTED 6UL

#define OSP2_QUEUE_META_MAGIC 0x0000513531554445UL
#define OSP2_QUEUE_ENTRY_MAGIC 0x00004A3531554445UL
#define OSP2_QUEUE_RESULT 0x6EC4E5DB9E1056CFUL
#define OSP2_QUEUE_COMPILER 0x1E3C373BAF48FAF7UL

#define OSP2_QUEUE_DECISION(state, reason, clear_leases, run_compute) \
    ((state) | ((reason) << 8) | ((clear_leases) << 16) | ((run_compute) << 17))

static osp2_u32 osp2_queue_read32(const osp2_u8* p) {
    return (osp2_u32)p[0] | ((osp2_u32)p[1] << 8) |
           ((osp2_u32)p[2] << 16) | ((osp2_u32)p[3] << 24);
}

static osp2_u64 osp2_queue_read64(const osp2_u8* p) {
    return (osp2_u64)osp2_queue_read32(p) |
           ((osp2_u64)osp2_queue_read32(p + 4) << 32);
}

static osp2_u32 osp2_queue_fnv(const osp2_u8* p, osp2_u64 count) {
    osp2_u32 value = 0x811C9DC5U;
    osp2_u64 index;
    for (index = 0; index < count; index = index + 1) {
        value = (value ^ p[index]) * 0x01000193U;
    }
    return value;
}

osp2_u64 osp2_queue_meta_admit(
    const osp2_u8* p,
    osp2_u64 entry_lba,
    osp2_u64 entry_count
) {
    if (osp2_queue_read64(p) != OSP2_QUEUE_META_MAGIC ||
        osp2_queue_read32(p + 8) != 1) {
        return OSP2_QUEUE_FORMAT;
    }
    if (osp2_queue_read32(p + 12) != entry_count ||
        osp2_queue_read32(p + 16) != entry_lba) {
        return OSP2_QUEUE_FORMAT;
    }
    if (osp2_queue_fnv(p, OSP2_QUEUE_SECTOR - 4) !=
        osp2_queue_read32(p + 508)) {
        return OSP2_QUEUE_FORMAT;
    }
    return OSP2_QUEUE_OK;
}

osp2_u64 osp2_queue_entry_action(
    const osp2_u8* p,
    osp2_u64 cpu_count,
    osp2_u64 free_pages
) {
    osp2_u64 state;
    if (osp2_queue_read64(p) != OSP2_QUEUE_ENTRY_MAGIC ||
        osp2_queue_read32(p + 8) != 1 ||
        osp2_queue_read32(p + 12) == 0 ||
        osp2_queue_read32(p + 12) > 4) {
        return OSP2_QUEUE_FORMAT;
    }
    if (osp2_queue_fnv(p, OSP2_QUEUE_SECTOR - 4) !=
        osp2_queue_read32(p + 508)) {
        return OSP2_QUEUE_FORMAT;
    }
    state = osp2_queue_read32(p + 24);
    if (state != OSP2_QUEUE_STATE_PENDING) {
        return OSP2_QUEUE_FORMAT;
    }
    if (osp2_queue_read64(p + 40) != OSP2_QUEUE_RESULT ||
        osp2_queue_read64(p + 48) != OSP2_QUEUE_COMPILER) {
        return OSP2_QUEUE_FORMAT;
    }
    if ((osp2_queue_read32(p + 28) & OSP2_QUEUE_FLAG_CANCEL_PENDING) != 0) {
        return OSP2_QUEUE_CANCEL;
    }
    if (osp2_queue_read32(p + 32) != 2 ||
        osp2_queue_read32(p + 36) != 2 ||
        cpu_count < 2 ||
        free_pages < 2) {
        return OSP2_QUEUE_RESOURCE;
    }
    return OSP2_QUEUE_OK;
}

osp2_u64 osp2_queue_existing_transition(
    osp2_u64 state,
    osp2_u64 result_matches
) {
    if (state == OSP2_QUEUE_STATE_COMPLETE) {
        if (result_matches != 0) {
            return OSP2_QUEUE_DECISION(OSP2_QUEUE_STATE_COMPLETE, 0, 0, 0);
        }
        return OSP2_QUEUE_DECISION(
            OSP2_QUEUE_STATE_FAILED, OSP2_QUEUE_FORMAT, 1, 0
        );
    }
    if (state == OSP2_QUEUE_STATE_FAILED) {
        return OSP2_QUEUE_DECISION(OSP2_QUEUE_STATE_FAILED, 0, 0, 0);
    }
    if (state == OSP2_QUEUE_STATE_CANCELLED) {
        return OSP2_QUEUE_DECISION(OSP2_QUEUE_STATE_CANCELLED, 0, 0, 0);
    }
    if (state == OSP2_QUEUE_STATE_RUNNING) {
        return OSP2_QUEUE_DECISION(
            OSP2_QUEUE_STATE_FAILED, OSP2_QUEUE_INTERRUPTED, 1, 0
        );
    }
    if (state == OSP2_QUEUE_STATE_PENDING) {
        return OSP2_QUEUE_DECISION(OSP2_QUEUE_STATE_PENDING, 0, 0, 1);
    }
    return OSP2_QUEUE_DECISION(
        OSP2_QUEUE_STATE_FAILED, OSP2_QUEUE_FORMAT, 1, 0
    );
}

osp2_u64 osp2_queue_terminal_transition(
    osp2_u64 action,
    osp2_u64 compute_ok,
    osp2_u64 result_matches
) {
    if (action == OSP2_QUEUE_CANCEL) {
        return OSP2_QUEUE_DECISION(
            OSP2_QUEUE_STATE_CANCELLED, OSP2_QUEUE_CANCEL, 1, 0
        );
    }
    if (action == OSP2_QUEUE_FORMAT || action == OSP2_QUEUE_RESOURCE) {
        return OSP2_QUEUE_DECISION(OSP2_QUEUE_STATE_FAILED, action, 1, 0);
    }
    if (action != OSP2_QUEUE_OK) {
        return OSP2_QUEUE_DECISION(
            OSP2_QUEUE_STATE_FAILED, OSP2_QUEUE_FORMAT, 1, 0
        );
    }
    if (compute_ok == 0 || result_matches == 0) {
        return OSP2_QUEUE_DECISION(
            OSP2_QUEUE_STATE_FAILED, OSP2_QUEUE_COMPUTE_FAILED, 1, 0
        );
    }
    return OSP2_QUEUE_DECISION(OSP2_QUEUE_STATE_COMPLETE, 0, 1, 0);
}
